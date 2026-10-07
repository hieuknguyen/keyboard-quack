#if defined(_WIN32) || defined(_WIN64)

#define COBJMACROS
#include <windows.h>
#include <uiautomation.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <oleauto.h>

#include "win32_uia.h"

#define UIA_POLL_INTERVAL_MS 25

static void release_range(IUIAutomationTextRange **range)
{
    if (*range) {
        IUIAutomationTextRange_Release(*range);
        *range = NULL;
    }
}

/* Runtime IDs distinguish focused controls hosted by the same top-level HWND. */
static ULONGLONG element_runtime_id_hash(IUIAutomationElement *element)
{
    SAFEARRAY *runtime_id = NULL;
    LONG lower = 0;
    LONG upper = -1;
    LONG *values = NULL;
    ULONGLONG hash = 1469598103934665603ULL;

    if (FAILED(IUIAutomationElement_GetRuntimeId(element, &runtime_id)) ||
        !runtime_id) {
        return 0;
    }

    if (SUCCEEDED(SafeArrayGetLBound(runtime_id, 1, &lower)) &&
        SUCCEEDED(SafeArrayGetUBound(runtime_id, 1, &upper)) &&
        upper >= lower &&
        SUCCEEDED(SafeArrayAccessData(runtime_id, (void **)&values))) {
        for (LONG i = 0; i <= upper - lower; i++) {
            hash ^= (ULONGLONG)(ULONG)values[i];
            hash *= 1099511628211ULL;
        }
        SafeArrayUnaccessData(runtime_id);
    } else {
        hash = 0;
    }

    SafeArrayDestroy(runtime_id);
    return hash;
}

static void query_focused_text(IUIAutomation *automation,
                               win32_uia_snapshot_t *snapshot)
{
    IUIAutomationElement *element = NULL;
    IUIAutomationTreeWalker *walker = NULL;
    IUIAutomationTextPattern2 *text_pattern = NULL;
    IUIAutomationTextRange *caret = NULL;
    IUIAutomationTextRange *document = NULL;
    IUIAutomationTextRange *prefix = NULL;
    IUIAutomationTextRangeArray *selections = NULL;
    IUIAutomationTextRange *selection = NULL;
    BOOL caret_active = FALSE;
    int selection_count = 0;
    int moved = 0;
    int comparison = 0;
    UIA_HWND native_window = NULL;
    int process_id = 0;
    ULONGLONG runtime_id_hash = 0;

    ZeroMemory(snapshot, sizeof(*snapshot));

    if (FAILED(IUIAutomation_GetFocusedElement(automation, &element)) || !element)
        goto done;

    /* Some providers put focus on a child node and expose TextPattern2 above it. */
    for (int depth = 0; depth < 8; depth++) {
        HRESULT pattern_result = IUIAutomationElement_GetCurrentPatternAs(
            element, UIA_TextPattern2Id, &IID_IUIAutomationTextPattern2,
            (void **)&text_pattern);
        if (SUCCEEDED(pattern_result) && text_pattern) break;
        if (text_pattern) IUIAutomationTextPattern2_Release(text_pattern);
        text_pattern = NULL;

        if (!walker && FAILED(IUIAutomation_get_ControlViewWalker(
                                  automation, &walker))) {
            goto done;
        }
        IUIAutomationElement *parent = NULL;
        if (FAILED(IUIAutomationTreeWalker_GetParentElement(
                walker, element, &parent)) || !parent) {
            goto done;
        }
        IUIAutomationElement_Release(element);
        element = parent;
    }
    if (!text_pattern) goto done;

    if (FAILED(IUIAutomationElement_get_CurrentProcessId(element, &process_id)) ||
        FAILED(IUIAutomationElement_get_CurrentNativeWindowHandle(element,
                                                                   &native_window))) {
        goto done;
    }
    runtime_id_hash = element_runtime_id_hash(element);

    if (FAILED(IUIAutomationTextPattern2_GetCaretRange(
            text_pattern, &caret_active, &caret)) || !caret || !caret_active) {
        goto done;
    }

    if (FAILED(IUIAutomationTextPattern2_get_DocumentRange(
            text_pattern, &document)) || !document ||
        FAILED(IUIAutomationTextRange_Clone(document, &prefix)) || !prefix ||
        FAILED(IUIAutomationTextRange_MoveEndpointByRange(
            prefix, TextPatternRangeEndpoint_End, caret,
            TextPatternRangeEndpoint_Start)) ||
        FAILED(IUIAutomationTextRange_MoveEndpointByUnit(
            prefix, TextPatternRangeEndpoint_Start, TextUnit_Character,
            INT_MAX, &moved)) || moved < 0) {
        goto done;
    }

    if (SUCCEEDED(IUIAutomationTextPattern2_GetSelection(text_pattern,
                                                          &selections)) &&
        selections &&
        SUCCEEDED(IUIAutomationTextRangeArray_get_Length(selections,
                                                          &selection_count))) {
        for (int i = 0; i < selection_count; i++) {
            if (SUCCEEDED(IUIAutomationTextRangeArray_GetElement(
                    selections, i, &selection)) && selection) {
                if (SUCCEEDED(IUIAutomationTextRange_CompareEndpoints(
                        selection, TextPatternRangeEndpoint_Start,
                        selection, TextPatternRangeEndpoint_End,
                        &comparison)) && comparison != 0) {
                    snapshot->has_selection = true;
                    release_range(&selection);
                    break;
                }
                release_range(&selection);
            }
        }
    }

    snapshot->available = true;
    snapshot->process_id = (DWORD)process_id;
    snapshot->element_window = (HWND)native_window;
    snapshot->element_runtime_id = runtime_id_hash;
    snapshot->caret_offset = (LONG)moved;

done:
    release_range(&selection);
    if (selections) IUIAutomationTextRangeArray_Release(selections);
    release_range(&prefix);
    release_range(&document);
    release_range(&caret);
    if (text_pattern) IUIAutomationTextPattern2_Release(text_pattern);
    if (walker) IUIAutomationTreeWalker_Release(walker);
    if (element) IUIAutomationElement_Release(element);
}

static void publish_snapshot(win32_uia_ctx_t *ctx,
                             const win32_uia_snapshot_t *snapshot)
{
    EnterCriticalSection(&ctx->snapshot_lock);
    ctx->snapshot = *snapshot;
    LeaveCriticalSection(&ctx->snapshot_lock);
}

static DWORD WINAPI uia_worker(void *parameter)
{
    win32_uia_ctx_t *ctx = (win32_uia_ctx_t *)parameter;
    IUIAutomation *automation = NULL;
    HRESULT com_result = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    bool com_initialized = SUCCEEDED(com_result);

    if (com_initialized) {
        CoCreateInstance(&CLSID_CUIAutomation, NULL, CLSCTX_INPROC_SERVER,
                         &IID_IUIAutomation, (void **)&automation);
    }

    while (WaitForSingleObject(ctx->stop_event, 0) != WAIT_OBJECT_0) {
        win32_uia_snapshot_t snapshot;
        ZeroMemory(&snapshot, sizeof(snapshot));
        /* Timestamp query start so a slow provider cannot make stale data look new. */
        snapshot.sampled_at_ms = GetTickCount64();
        if (automation) query_focused_text(automation, &snapshot);
        publish_snapshot(ctx, &snapshot);

        if (WaitForSingleObject(ctx->stop_event,
                                UIA_POLL_INTERVAL_MS) == WAIT_OBJECT_0) {
            break;
        }
    }

    if (automation) IUIAutomation_Release(automation);
    if (com_initialized) CoUninitialize();
    return 0;
}

int win32_uia_start(win32_uia_ctx_t *ctx)
{
    if (!ctx) return -1;
    ZeroMemory(ctx, sizeof(*ctx));
    InitializeCriticalSection(&ctx->snapshot_lock);
    ctx->lock_initialized = true;

    ctx->stop_event = CreateEventA(NULL, TRUE, FALSE, NULL);
    if (!ctx->stop_event) {
        DeleteCriticalSection(&ctx->snapshot_lock);
        ctx->lock_initialized = false;
        return -1;
    }

    ctx->worker = CreateThread(NULL, 0, uia_worker, ctx, 0, NULL);
    if (!ctx->worker) {
        CloseHandle(ctx->stop_event);
        ctx->stop_event = NULL;
        DeleteCriticalSection(&ctx->snapshot_lock);
        ctx->lock_initialized = false;
        return -1;
    }
    return 0;
}

bool win32_uia_get_snapshot(win32_uia_ctx_t *ctx,
                            win32_uia_snapshot_t *snapshot)
{
    if (!ctx || !snapshot || !ctx->lock_initialized) return false;
    EnterCriticalSection(&ctx->snapshot_lock);
    *snapshot = ctx->snapshot;
    LeaveCriticalSection(&ctx->snapshot_lock);
    return true;
}

void win32_uia_stop(win32_uia_ctx_t *ctx)
{
    if (!ctx) return;

    if (ctx->stop_event) SetEvent(ctx->stop_event);
    if (ctx->worker) {
        DWORD wait = WaitForSingleObject(ctx->worker, 2000);
        CloseHandle(ctx->worker);
        ctx->worker = NULL;

        /* A provider can stall an accessibility call. Keep the shared objects
         * alive until process exit if that happens, avoiding a worker UAF. */
        if (wait == WAIT_TIMEOUT) return;
    }

    if (ctx->stop_event) {
        CloseHandle(ctx->stop_event);
        ctx->stop_event = NULL;
    }
    if (ctx->lock_initialized) {
        DeleteCriticalSection(&ctx->snapshot_lock);
        ctx->lock_initialized = false;
    }
}

#endif /* _WIN32 */
