#pragma once
#include "../app.h"
#include <adwaita.h>

class MainWindow {
public:
    MainWindow(App& app, AdwApplication* adw_app);
    GtkWidget* widget() { return GTK_WIDGET(window_); }

private:
    GtkWidget* build_sidebar();
    void refresh_devices(const std::map<std::string, NetworkDevice>& devs);

    void build_transfer_panel();
    void refresh_transfers(const std::vector<TransferItem>& items);
    void refresh_conn_state(bool connected);

    void show_settings();
    void pick_files();

    App&                  app_;
    AdwApplicationWindow* window_{nullptr};
    AdwWindowTitle*       win_title_{nullptr};

    GtkWidget* status_label_{nullptr};
    GtkWidget* disconnect_btn_{nullptr};
    GtkWidget* devices_list_{nullptr};
    GtkWidget* manual_revealer_{nullptr};
    GtkWidget* manual_host_{nullptr};

    AdwToastOverlay* toast_overlay_{nullptr};
    GtkWidget* text_entry_{nullptr};
    GtkWidget* send_btn_{nullptr};
    GtkWidget* attach_btn_{nullptr};
    GtkWidget* transfers_list_{nullptr};
    GtkWidget* transfer_content_{nullptr};

    std::map<std::string, GtkWidget*> transfer_rows_;
    struct RowWidgets {
        GtkWidget* progress{nullptr};
        GtkWidget* sub_label{nullptr};
        GtkWidget* open_btn{nullptr};
    };
    std::map<std::string, RowWidgets> row_widgets_;

    std::map<std::string, NetworkDevice> last_devices_;
};
