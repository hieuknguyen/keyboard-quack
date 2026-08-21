# keyboard-quack

Bộ gõ tiếng Việt Telex độc lập, siêu nhẹ và bảo mật cao cho Linux (hỗ trợ cả **Wayland** và **X11**).

Khác với các bộ gõ truyền thống dựa trên IBus/Fcitx, `keyboard-quack` hoạt động trực tiếp ở tầng nhân Linux (`evdev` & `uinput`), mang lại trải nghiệm gõ tức thì, không giật lag và tương thích 100% với mọi ứng dụng (Terminal, IDE, Trình duyệt, Game...).

---

## Tính năng nổi bật

- **Tương thích toàn diện:** Chạy mượt mà trên cả Wayland (GNOME, KDE) và X11.
- **Chuẩn hóa chính tả Telex:** Xử lý chính xác các trường hợp đặt dấu phức tạp (`toán`, `hoàng`, `khoác`, `giá`, `giảm`, `giẻ`, `quở`, `nguyễn`...).
- **Xóa dấu thanh bằng phím `z`:** Nhấn `z` để xóa dấu thanh (`toán` + `z` $\rightarrow$ `toan`).
- **Phục hồi dấu thông minh:** Nhấn lặp lại phím dấu để quay về ký tự gốc (`as` $\rightarrow$ `as`, `aa` $\rightarrow$ `aa`).
- **Diff-based Backspace:** Chỉ xóa lùi số lượng ký tự tối thiểu khi sửa dấu, không giật lag màn hình.
- **Tự động chạy ngầm:** Được quản lý như một dịch vụ hệ thống `systemd`, tự khởi động cùng máy tính mà không đòi hỏi thao tác thủ công.
- **Giao diện cấu hình (GUI):** Tích hợp khay hệ thống (System Tray) bằng Qt5/Qt6.

---

## Cài đặt (1-Click Install)

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

---

## Hướng dẫn sử dụng

- **Bật / Tắt tiếng Việt:** Nhấn tổ hợp phím **`Ctrl + Space`** (hoặc `Ctrl + Shift`).
- **Mở giao diện cài đặt:** 
  - Chạy lệnh `quack-config` trong Terminal.
  - Hoặc tìm **"keyboard-quack"** trong Menu ứng dụng.
  - Hoặc click vào biểu tượng bàn phím ở khay hệ thống (System Tray).

---

## Gỡ cài đặt (Uninstall)

Nếu muốn gỡ bỏ hoàn toàn bộ gõ khỏi hệ thống:

```bash
sudo ./uninstall.sh
```

---

## Giấy phép

Phát triển bởi cộng đồng mã nguồn mở. Tự do sử dụng và đóng góp.
