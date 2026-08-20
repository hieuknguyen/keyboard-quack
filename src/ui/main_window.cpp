#include "main_window.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QMessageBox>
#include <QApplication>
#include <QMenu>
#include <QAction>

extern "C" {
#include "../config/config.h"
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), vn_enabled(true)
{
    setup_ui();
    load_config();

    /* System tray */
    tray_icon = new QSystemTrayIcon(this);
    QMenu *tray_menu = new QMenu(this);

    QAction *toggle_action = tray_menu->addAction("Enable/Disable Vietnamese");
    connect(toggle_action, &QAction::triggered, this, &MainWindow::toggle_vn);

    QAction *show_action = tray_menu->addAction("Show Settings");
    connect(show_action, &QAction::triggered, this, &MainWindow::show);

    QAction *quit_action = tray_menu->addAction("Quit");
    connect(quit_action, &QAction::triggered, qApp, &QApplication::quit);

    tray_icon->setContextMenu(tray_menu);
    connect(tray_icon, &QSystemTrayIcon::activated,
            this, &MainWindow::tray_icon_activated);

    update_tray_icon();
    tray_icon->show();
}

MainWindow::~MainWindow()
{
}

void MainWindow::setup_ui()
{
    setWindowTitle("keyboard-quack Settings");
    setFixedSize(400, 350);

    QWidget *central = new QWidget(this);
    setCentralWidget(central);

    QVBoxLayout *main_layout = new QVBoxLayout(central);

    /* Title */
    QLabel *title = new QLabel("<h2>keyboard-quack</h2>"
                               "<p>Vietnamese Telex Input Method</p>");
    title->setAlignment(Qt::AlignCenter);
    main_layout->addWidget(title);

    /* Input Method */
    QGroupBox *method_group = new QGroupBox("Input Method");
    QFormLayout *method_layout = new QFormLayout(method_group);

    method_combo = new QComboBox();
    method_combo->addItem("Telex", 0);
    method_combo->addItem("VNI", 1);
    method_layout->addRow("Method:", method_combo);

    toggle_combo = new QComboBox();
    toggle_combo->addItem("Ctrl+Space", 0);
    toggle_combo->addItem("CapsLock", 1);
    toggle_combo->addItem("Grave (`)", 2);
    method_layout->addRow("Toggle Key:", toggle_combo);

    main_layout->addWidget(method_group);

    /* Options */
    QGroupBox *opt_group = new QGroupBox("Options");
    QVBoxLayout *opt_layout = new QVBoxLayout(opt_group);

    auto_start_cb = new QCheckBox("Auto-start on login");
    opt_layout->addWidget(auto_start_cb);

    show_tray_cb = new QCheckBox("Show system tray icon");
    show_tray_cb->setChecked(true);
    opt_layout->addWidget(show_tray_cb);

    terminal_cb = new QCheckBox("Enable in terminal");
    terminal_cb->setChecked(true);
    opt_layout->addWidget(terminal_cb);

    password_cb = new QCheckBox("Enable in password fields");
    opt_layout->addWidget(password_cb);

    debug_cb = new QCheckBox("Debug mode");
    opt_layout->addWidget(debug_cb);

    main_layout->addWidget(opt_group);

    /* Status */
    status_label = new QLabel("Status: Vietnamese ENABLED");
    status_label->setStyleSheet("color: green; font-weight: bold;");
    main_layout->addWidget(status_label);

    /* Buttons */
    QHBoxLayout *btn_layout = new QHBoxLayout();

    reset_btn = new QPushButton("Reset Defaults");
    connect(reset_btn, &QPushButton::clicked, this, &MainWindow::reset_defaults);
    btn_layout->addWidget(reset_btn);

    save_btn = new QPushButton("Save");
    save_btn->setDefault(true);
    connect(save_btn, &QPushButton::clicked, this, &MainWindow::save_config);
    btn_layout->addWidget(save_btn);

    main_layout->addLayout(btn_layout);
}

void MainWindow::load_config()
{
    config_load(&config, NULL);

    method_combo->setCurrentIndex(config.input_method);
    toggle_combo->setCurrentIndex(config.toggle_key);
    auto_start_cb->setChecked(config.auto_start);
    show_tray_cb->setChecked(config.show_tray);
    terminal_cb->setChecked(config.enable_terminal);
    password_cb->setChecked(config.enable_password);
    debug_cb->setChecked(config.debug);
}

void MainWindow::save_config()
{
    config.input_method = method_combo->currentData().toInt();
    config.toggle_key = toggle_combo->currentData().toInt();
    config.auto_start = auto_start_cb->isChecked();
    config.show_tray = show_tray_cb->isChecked();
    config.enable_terminal = terminal_cb->isChecked();
    config.enable_password = password_cb->isChecked();
    config.debug = debug_cb->isChecked();

    if (config_save(&config, NULL) == 0) {
        QMessageBox::information(this, "Saved", "Configuration saved successfully!");
    } else {
        QMessageBox::warning(this, "Error", "Failed to save configuration.");
    }
}

void MainWindow::reset_defaults()
{
    config_defaults(&config);
    load_config();
    QMessageBox::information(this, "Reset", "Configuration reset to defaults.");
}

void MainWindow::toggle_vn()
{
    vn_enabled = !vn_enabled;
    update_tray_icon();
}

void MainWindow::update_tray_icon()
{
    if (vn_enabled) {
        tray_icon->setIcon(QIcon::fromTheme("input-keyboard"));
        tray_icon->setToolTip("keyboard-quack: Vietnamese ON");
        status_label->setText("Status: Vietnamese ENABLED");
        status_label->setStyleSheet("color: green; font-weight: bold;");
    } else {
        tray_icon->setIcon(QIcon::fromTheme("input-keyboard"));
        tray_icon->setToolTip("keyboard-quack: Vietnamese OFF");
        status_label->setText("Status: Vietnamese DISABLED");
        status_label->setStyleSheet("color: red; font-weight: bold;");
    }
}

void MainWindow::tray_icon_activated(QSystemTrayIcon::ActivationReason reason)
{
    if (reason == QSystemTrayIcon::DoubleClick) {
        toggle_vn();
    } else if (reason == QSystemTrayIcon::Trigger) {
        show();
        raise();
        activateWindow();
    }
}

#include "main_window.moc"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("keyboard-quack");
    app.setQuitOnLastWindowClosed(false);

    MainWindow w;
    w.show();
    return app.exec();
}
