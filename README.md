# keyboard-quack

Bộ gõ tiếng Việt Telex độc lập, siêu nhẹ, đa nền tảng (**Linux** & **Windows**).

- **Trên Linux:** Hoạt động trực tiếp ở tầng kernel (`evdev` & `uinput`), hỗ trợ cả **Wayland** (thông qua dead-keys trên tầng `AltGr`) và **X11**, không phụ thuộc IBus hay Fcitx.
- **Trên Windows:** Hoạt động thông qua **Win32 Low-Level Keyboard Hook** và **SendInput (Native Unicode UTF-16)** với cơ chế batching nguyên tử, gõ tức thì, không giật lag.
- **Tự động nhận diện hệ điều hành:** Ứng dụng tự động kiểm tra hệ điều hành đang chạy (Windows hay Linux) khi khởi động để chọn đúng backend tương ứng mà không cần cấu hình thủ công.

---

## Tính năng nổi bật

- **Đa nền tảng thông minh:** Tự động phát hiện và kích hoạt backend phù hợp cho Windows và Linux.
- **Tương thích toàn diện:** Chạy mượt mà trên Windows (10/11), Linux Wayland (GNOME, KDE) và X11.
- **Chuẩn hóa chính tả Telex:** Xử lý chính xác các trường hợp đặt dấu phức tạp (`toán`, `hoàng`, `khoác`, `giá`, `giảm`, `giẻ`, `quở`, `nguyễn`...).
- **Xóa dấu thanh bằng phím `z`:** Nhấn `z` để xóa dấu thanh (`toán` + `z` $\rightarrow$ `toan`).
- **Phục hồi dấu thông minh:** Nhấn lặp lại phím dấu để quay về ký tự gốc (`as` $\rightarrow$ `as`, `aa` $\rightarrow$ `aa`, `dd` $\rightarrow$ `dd`, `aw` $\rightarrow$ `aw`).
- **Diff-based Backspace:** Chỉ xóa lùi số lượng ký tự tối thiểu khi sửa dấu, không giật lag màn hình.
- **Giao diện cấu hình (GUI):** Tích hợp khay hệ thống (System Tray) bằng Qt5/Qt6.

---

## Hướng dẫn cài đặt & Chạy

### 🐧 Trên Linux (1-Click Install)

Chạy lệnh sau để tự động biên dịch và cài đặt vào hệ thống:

```bash
sudo ./install.sh
```

Script cài đặt sẽ tự động:
1. Biên dịch mã nguồn tối ưu.
2. Cài đặt các file thực thi `quack` và `quack-config` vào `/usr/local/bin`.
3. Cấu hình quyền truy cập phần cứng (`udev rules` & nhóm `input`).
4. Kích hoạt dịch vụ chạy ngầm `systemd` tự khởi động cùng máy tính.
5. Tạo lối tắt ứng dụng trong Application Menu.

### 🪟 Trên Windows

#### Bản cài đặt

1. Cài **MinGW-w64 GCC** cùng `windres` (MSYS2 UCRT64 là một lựa chọn), hoặc thêm chúng vào `PATH`.
2. Chạy `build.bat` từ thư mục project (hoặc gọi trực tiếp `build-windows-installer.bat`):
   ```cmd
   build.bat
   ```
3. Chạy `build-windows\keyboard-quack-setup.exe`. Bộ cài cài cho tài khoản hiện tại vào `%LOCALAPPDATA%\Programs\keyboard-quack`, tạo lối tắt trong Start Menu và đăng ký trong **Installed apps / Apps & features**. Không cần quyền quản trị.
4. Có thể gỡ ứng dụng từ danh sách ứng dụng Windows, lối tắt **Uninstall keyboard-quack**, hoặc menu biểu tượng khay hệ thống. Trình gỡ xóa file đã cài, lối tắt và đăng ký chạy cùng Windows; cấu hình trong AppData được giữ lại.

#### Bản portable

Để tạo riêng `quack.exe` portable, build thủ công bằng CMake:
   ```cmd
   cmake -S . -B build-windows
   cmake --build build-windows --config Release
   ```
Chạy `quack.exe` trong thư mục `build-windows\Release` (Visual Studio) hoặc `build-windows` (generator một cấu hình). Ứng dụng chạy nền và hiện biểu tượng ở khay hệ thống, không mở cửa sổ terminal.

Ứng dụng bắt phím bằng Win32 Low-Level Keyboard Hook và gửi chữ tiếng Việt qua `SendInput`. Bấm biểu tượng khay hệ thống để bật/tắt; bấm chuột phải để mở menu, chọn **Toggle shortcut**, thoát, thêm/gỡ chạy cùng Windows, hoặc gỡ ứng dụng. Phím tắt bật/tắt mặc định là **Off**; có thể chọn `Ctrl+Space`, `Ctrl+Shift`, `Win+Space`, `Ctrl+Alt+V`, `Ctrl+Shift+V`, `Alt+Space`, `CapsLock` hoặc `Grave`. Khi app chạy, `Ctrl+Shift` và `Win+Space` không còn tự đổi layout Windows nếu chưa được chọn làm phím tắt của app. Giao diện `quack-config` chỉ được tạo nếu Qt5 hoặc Qt6 Widgets đã có sẵn khi cấu hình CMake.

Trong menu khay hệ thống, chọn **Check for updates** để kiểm tra GitHub Releases của repo `hieuknguyen/keyboard-quack`. Tính năng cập nhật trong ứng dụng dành cho bản đã cài; nếu có phiên bản mới, ứng dụng tải `keyboard-quack-setup.exe`, xác minh SHA-256 rồi mở bộ cài để nâng cấp. Khi phát hành bản mới, chỉ cần đổi `QUACK_VERSION` trong `src/version.h`, rồi push tag `vMAJOR.MINOR.PATCH` khớp với giá trị đó. CMake, ứng dụng và installer tự đọc cùng phiên bản này; workflow `.github/workflows/windows-release.yml` kiểm tra tag rồi tự build setup và tạo GitHub Release.

Trên Windows, ứng dụng xóa trạng thái Telex khi chuyển cửa sổ ứng dụng hoặc khi click chuột, phòng trường hợp vị trí con trỏ/vùng chọn đã đổi. UI Automation đồng bộ thêm vị trí con trỏ và vùng chọn khi control cung cấp `TextPattern2`; control không hỗ trợ thông tin này vẫn dùng theo dõi phím làm phương án dự phòng. Phím điều hướng cũng xóa trạng thái Telex cũ để tránh Backspace sửa nhầm chữ ở vị trí trước đó.

---

## Hướng dẫn sử dụng

- **Bật / Tắt tiếng Việt:** Mặc định dùng biểu tượng khay hệ thống. Chọn phím tắt tại **Toggle shortcut** trong menu chuột phải trên Windows, hoặc trong `quack-config` nếu đã cài GUI.
- **Mở giao diện cài đặt:** 
  - Chạy lệnh `quack-config` trong Terminal/Command Prompt.
  - Hoặc tìm **"keyboard-quack"** trong Menu ứng dụng.
  - Hoặc click vào biểu tượng bàn phím ở khay hệ thống (System Tray).

---

## Gỡ cài đặt trên Linux (Uninstall)

```bash
sudo ./uninstall.sh
```

---

## Giấy phép

Phát triển bởi cộng đồng mã nguồn mở. Tự do sử dụng và đóng góp.
