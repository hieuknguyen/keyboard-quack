#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include <QMainWindow>
#include <QComboBox>
#include <QCheckBox>
#include <QPushButton>
#include <QLabel>
#include <QSystemTrayIcon>

extern "C" {
#include "../config/config.h"
}

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void save_config();
    void reset_defaults();
    void toggle_vn();
    void tray_icon_activated(QSystemTrayIcon::ActivationReason reason);

private:
    void setup_ui();
    void load_config();
    void update_tray_icon();

    quack_config_t config;

    QComboBox *method_combo;
    QComboBox *toggle_combo;
    QCheckBox *auto_start_cb;
    QCheckBox *show_tray_cb;
    QCheckBox *terminal_cb;
    QCheckBox *password_cb;
    QCheckBox *debug_cb;
    QPushButton *save_btn;
    QPushButton *reset_btn;
    QLabel *status_label;

    QSystemTrayIcon *tray_icon;
    bool vn_enabled;
};

#endif /* MAIN_WINDOW_H */
