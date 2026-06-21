#include "main_window.h"
#include <cstring>
#include <string>
#include <sstream>
#include <iomanip>
#include <set>

static void apply_theme(const std::string& mode) {
    AdwStyleManager* mgr = adw_style_manager_get_default();
    if (mode == "dark")
        adw_style_manager_set_color_scheme(mgr, ADW_COLOR_SCHEME_FORCE_DARK);
    else if (mode == "light")
        adw_style_manager_set_color_scheme(mgr, ADW_COLOR_SCHEME_FORCE_LIGHT);
    else
        adw_style_manager_set_color_scheme(mgr, ADW_COLOR_SCHEME_DEFAULT);
}

static std::string fmt_size(int64_t bytes) {
    if (bytes < 1024) return std::to_string(bytes) + " B";
    if (bytes < 1024*1024) {
        std::ostringstream ss;
        ss << std::fixed << std::setprecision(1) << bytes / 1024.0 << " KB";
        return ss.str();
    }
    if (bytes < 1024LL*1024*1024) {
        std::ostringstream ss;
        ss << std::fixed << std::setprecision(1) << bytes / (1024.0*1024) << " MB";
        return ss.str();
    }
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(2) << bytes / (1024.0*1024*1024) << " GB";
    return ss.str();
}

static GtkWidget* make_label(const char* text, const char* css = nullptr,
                              GtkAlign halign = GTK_ALIGN_START) {
    GtkWidget* w = gtk_label_new(text);
    gtk_label_set_xalign(GTK_LABEL(w), 0.0f);
    gtk_widget_set_halign(w, halign);
    if (css) gtk_widget_add_css_class(w, css);
    return w;
}

MainWindow::MainWindow(App& app, AdwApplication* adw_app) : app_(app) {
    window_ = ADW_APPLICATION_WINDOW(adw_application_window_new(GTK_APPLICATION(adw_app)));
    gtk_window_set_title(GTK_WINDOW(window_), "Fly");
    gtk_window_set_default_size(GTK_WINDOW(window_), 960, 640);

    GtkWidget* root = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    adw_application_window_set_content(window_, root);

    AdwHeaderBar* hbar = ADW_HEADER_BAR(adw_header_bar_new());
    win_title_ = ADW_WINDOW_TITLE(adw_window_title_new("Fly", ""));
    adw_header_bar_set_title_widget(hbar, GTK_WIDGET(win_title_));

    GtkWidget* settings_btn = gtk_button_new_from_icon_name("emblem-system-symbolic");
    gtk_widget_add_css_class(settings_btn, "flat");
    gtk_widget_set_tooltip_text(settings_btn, "Settings");
    g_signal_connect_swapped(settings_btn, "clicked",
        G_CALLBACK(+[](MainWindow* self) { self->show_settings(); }), this);
    adw_header_bar_pack_end(hbar, settings_btn);
    gtk_box_append(GTK_BOX(root), GTK_WIDGET(hbar));

    GtkWidget* paned = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_widget_set_vexpand(paned, TRUE);
    gtk_paned_set_position(GTK_PANED(paned), 240);
    gtk_paned_set_shrink_start_child(GTK_PANED(paned), FALSE);
    gtk_paned_set_shrink_end_child(GTK_PANED(paned), FALSE);
    gtk_box_append(GTK_BOX(root), paned);

    GtkWidget* sidebar_scroll = gtk_scrolled_window_new();
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sidebar_scroll),
                                   GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_widget_set_size_request(sidebar_scroll, 220, -1);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(sidebar_scroll), build_sidebar());
    gtk_paned_set_start_child(GTK_PANED(paned), sidebar_scroll);

    build_transfer_panel();
    toast_overlay_ = ADW_TOAST_OVERLAY(adw_toast_overlay_new());
    adw_toast_overlay_set_child(toast_overlay_, transfer_content_);
    gtk_widget_set_hexpand(GTK_WIDGET(toast_overlay_), TRUE);
    gtk_paned_set_end_child(GTK_PANED(paned), GTK_WIDGET(toast_overlay_));

    GtkDropTarget* dt = gtk_drop_target_new(GDK_TYPE_FILE_LIST, GDK_ACTION_COPY);
    g_signal_connect(dt, "drop",
        G_CALLBACK(+[](GtkDropTarget*, const GValue* val,
                        gdouble, gdouble, gpointer ud) -> gboolean {
            auto* self = static_cast<MainWindow*>(ud);
            if (!self->app_.active_conn()) return FALSE;
            auto* list = static_cast<GSList*>(g_value_get_boxed(val));
            for (auto* l = list; l; l = l->next) {
                char* path = g_file_get_path(G_FILE(l->data));
                if (path) { self->app_.send_file(path); g_free(path); }
            }
            return TRUE;
        }), this);
    gtk_widget_add_controller(GTK_WIDGET(window_), GTK_EVENT_CONTROLLER(dt));

    app_.on_devices_changed = [this](auto devs) { refresh_devices(devs); };
    app_.on_connection_changed = [this](auto conn) {
        bool c = (conn != nullptr);
        refresh_conn_state(c);
        if (c) {
            adw_window_title_set_subtitle(win_title_, conn->peer_name.c_str());
            gtk_label_set_text(GTK_LABEL(status_label_),
                ("Connected · " + conn->peer_name).c_str());
        } else {
            adw_window_title_set_subtitle(win_title_, "");
            gtk_label_set_text(GTK_LABEL(status_label_), "Not connected");
        }
    };
    app_.on_status_changed    = [this](auto s) {
        gtk_label_set_text(GTK_LABEL(status_label_), s.c_str());
    };
    app_.on_transfers_changed = [this](auto items) { refresh_transfers(items); };

    apply_theme(app_.settings().theme_mode);
}

GtkWidget* MainWindow::build_sidebar() {
    GtkWidget* box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

    GtkWidget* nearby_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    gtk_widget_set_margin_start(nearby_row, 16);
    gtk_widget_set_margin_end(nearby_row, 12);
    gtk_widget_set_margin_top(nearby_row, 16);
    gtk_widget_set_margin_bottom(nearby_row, 8);

    GtkWidget* nearby_lbl = make_label("Nearby", "heading");
    gtk_widget_set_hexpand(nearby_lbl, TRUE);

    GtkWidget* spinner = gtk_spinner_new();
    gtk_spinner_start(GTK_SPINNER(spinner));
    gtk_widget_add_css_class(spinner, "dim-label");

    gtk_box_append(GTK_BOX(nearby_row), nearby_lbl);
    gtk_box_append(GTK_BOX(nearby_row), spinner);
    gtk_box_append(GTK_BOX(box), nearby_row);

    devices_list_ = gtk_list_box_new();
    gtk_list_box_set_selection_mode(GTK_LIST_BOX(devices_list_), GTK_SELECTION_NONE);
    gtk_widget_add_css_class(devices_list_, "boxed-list");
    gtk_widget_set_margin_start(devices_list_, 12);
    gtk_widget_set_margin_end(devices_list_, 12);

    g_signal_connect(devices_list_, "row-activated",
        G_CALLBACK(+[](GtkListBox*, GtkListBoxRow* row, gpointer ud) {
            auto* self = static_cast<MainWindow*>(ud);
            auto* id = static_cast<std::string*>(
                g_object_get_data(G_OBJECT(row), "device-id"));
            if (!id) return;
            auto it = self->last_devices_.find(*id);
            if (it != self->last_devices_.end())
                self->app_.connect_to(it->second);
        }), this);

    GtkWidget* ph = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_widget_set_margin_top(ph, 24);
    gtk_widget_set_margin_bottom(ph, 24);
    GtkWidget* ph_spin = gtk_spinner_new();
    gtk_spinner_start(GTK_SPINNER(ph_spin));
    gtk_widget_set_halign(ph_spin, GTK_ALIGN_CENTER);
    GtkWidget* ph_lbl = make_label("Scanning…", "dim-label", GTK_ALIGN_CENTER);
    gtk_widget_add_css_class(ph_lbl, "caption");
    gtk_widget_set_halign(ph_lbl, GTK_ALIGN_CENTER);
    gtk_box_append(GTK_BOX(ph), ph_spin);
    gtk_box_append(GTK_BOX(ph), ph_lbl);
    gtk_list_box_set_placeholder(GTK_LIST_BOX(devices_list_), ph);

    gtk_box_append(GTK_BOX(box), devices_list_);

    GtkWidget* manual_hdr = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_set_margin_start(manual_hdr, 12);
    gtk_widget_set_margin_end(manual_hdr, 12);
    gtk_widget_set_margin_top(manual_hdr, 12);

    GtkWidget* manual_toggle = gtk_toggle_button_new();
    GtkWidget* toggle_inner = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    gtk_box_append(GTK_BOX(toggle_inner), gtk_image_new_from_icon_name("list-add-symbolic"));
    gtk_box_append(GTK_BOX(toggle_inner), gtk_label_new("Manual connect"));
    gtk_button_set_child(GTK_BUTTON(manual_toggle), toggle_inner);
    gtk_widget_add_css_class(manual_toggle, "flat");
    gtk_widget_set_hexpand(manual_toggle, TRUE);
    gtk_widget_set_halign(manual_toggle, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(manual_hdr), manual_toggle);
    gtk_box_append(GTK_BOX(box), manual_hdr);

    manual_revealer_ = gtk_revealer_new();
    gtk_revealer_set_transition_type(GTK_REVEALER(manual_revealer_),
                                     GTK_REVEALER_TRANSITION_TYPE_SLIDE_DOWN);
    GtkWidget* manual_inner = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_widget_set_margin_start(manual_inner, 12);
    gtk_widget_set_margin_end(manual_inner, 12);
    gtk_widget_set_margin_top(manual_inner, 8);
    gtk_widget_set_margin_bottom(manual_inner, 4);

    manual_host_ = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(manual_host_), "host:port");
    GtkWidget* conn_btn = gtk_button_new_with_label("Connect");
    gtk_widget_add_css_class(conn_btn, "suggested-action");
    gtk_widget_add_css_class(conn_btn, "pill");
    g_signal_connect_swapped(conn_btn, "clicked",
        G_CALLBACK(+[](MainWindow* self) {
            const char* txt = gtk_editable_get_text(GTK_EDITABLE(self->manual_host_));
            std::string s{txt};
            auto colon = s.rfind(':');
            std::string host = (colon != std::string::npos) ? s.substr(0, colon) : s;
            int port = 5800;
            if (colon != std::string::npos)
                try { port = std::stoi(s.substr(colon + 1)); } catch (...) {}
            if (!host.empty()) self->app_.connect_manual(host, port);
        }), this);
    gtk_box_append(GTK_BOX(manual_inner), manual_host_);
    gtk_box_append(GTK_BOX(manual_inner), conn_btn);
    gtk_revealer_set_child(GTK_REVEALER(manual_revealer_), manual_inner);

    g_signal_connect(manual_toggle, "toggled",
        G_CALLBACK(+[](GtkToggleButton* btn, gpointer ud) {
            gtk_revealer_set_reveal_child(GTK_REVEALER(ud),
                gtk_toggle_button_get_active(btn));
        }), manual_revealer_);
    gtk_box_append(GTK_BOX(box), manual_revealer_);

    GtkWidget* spacer = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_vexpand(spacer, TRUE);
    gtk_box_append(GTK_BOX(box), spacer);

    gtk_box_append(GTK_BOX(box), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL));

    GtkWidget* bottom = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_widget_set_margin_start(bottom, 12);
    gtk_widget_set_margin_end(bottom, 12);
    gtk_widget_set_margin_top(bottom, 8);
    gtk_widget_set_margin_bottom(bottom, 12);

    status_label_ = make_label("Starting…", "dim-label");
    gtk_widget_add_css_class(status_label_, "caption");
    gtk_label_set_wrap(GTK_LABEL(status_label_), TRUE);

    disconnect_btn_ = gtk_button_new_with_label("Disconnect");
    gtk_widget_add_css_class(disconnect_btn_, "destructive-action");
    gtk_widget_add_css_class(disconnect_btn_, "pill");
    gtk_widget_set_visible(disconnect_btn_, FALSE);
    g_signal_connect_swapped(disconnect_btn_, "clicked",
        G_CALLBACK(+[](MainWindow* self) { self->app_.disconnect(); }), this);

    gtk_box_append(GTK_BOX(bottom), status_label_);
    gtk_box_append(GTK_BOX(bottom), disconnect_btn_);
    gtk_box_append(GTK_BOX(box), bottom);

    return box;
}

void MainWindow::refresh_devices(const std::map<std::string, NetworkDevice>& devs) {
    last_devices_ = devs;

    while (GtkWidget* child = gtk_widget_get_first_child(devices_list_))
        gtk_list_box_remove(GTK_LIST_BOX(devices_list_), child);

    auto active = app_.active_conn();

    for (auto& [id, dev] : devs) {
        AdwActionRow* row = ADW_ACTION_ROW(adw_action_row_new());
        adw_preferences_row_set_title(ADW_PREFERENCES_ROW(row), dev.name.c_str());
        adw_action_row_set_subtitle(row, dev.host.c_str());

        const char* icon = "computer-symbolic";
        if (dev.platform == "android") icon = "phone-symbolic";
        else if (dev.platform == "ios") icon = "phone-apple-iphone-symbolic";
        GtkWidget* img = gtk_image_new_from_icon_name(icon);
        gtk_image_set_pixel_size(GTK_IMAGE(img), 28);
        adw_action_row_add_prefix(row, img);

        bool is_active = active && (active->peer_name == dev.name);
        GtkWidget* indicator = is_active
            ? gtk_image_new_from_icon_name("emblem-ok-symbolic")
            : gtk_image_new_from_icon_name("go-next-symbolic");
        gtk_widget_add_css_class(indicator, is_active ? "success" : "dim-label");
        adw_action_row_add_suffix(row, indicator);

        g_object_set_data_full(G_OBJECT(row), "device-id",
            new std::string(id),
            [](gpointer p) { delete static_cast<std::string*>(p); });
        gtk_list_box_append(GTK_LIST_BOX(devices_list_), GTK_WIDGET(row));
    }
}

void MainWindow::build_transfer_panel() {
    transfer_content_ = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

    GtkWidget* scroll = gtk_scrolled_window_new();
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll),
                                   GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_widget_set_vexpand(scroll, TRUE);

    transfers_list_ = gtk_list_box_new();
    gtk_list_box_set_selection_mode(GTK_LIST_BOX(transfers_list_), GTK_SELECTION_NONE);
    gtk_widget_add_css_class(transfers_list_, "boxed-list");
    gtk_widget_set_margin_start(transfers_list_, 12);
    gtk_widget_set_margin_end(transfers_list_, 12);
    gtk_widget_set_margin_top(transfers_list_, 12);
    gtk_widget_set_margin_bottom(transfers_list_, 12);

    GtkWidget* empty = adw_status_page_new();
    adw_status_page_set_icon_name(ADW_STATUS_PAGE(empty), "network-workgroup-symbolic");
    adw_status_page_set_title(ADW_STATUS_PAGE(empty), "No Transfers");
    adw_status_page_set_description(ADW_STATUS_PAGE(empty),
        "Connect to a device, then send a message or drop files here.");
    gtk_list_box_set_placeholder(GTK_LIST_BOX(transfers_list_), empty);

    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroll), transfers_list_);
    gtk_box_append(GTK_BOX(transfer_content_), scroll);

    gtk_box_append(GTK_BOX(transfer_content_),
                   gtk_separator_new(GTK_ORIENTATION_HORIZONTAL));

    GtkWidget* input_bar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    gtk_widget_set_margin_start(input_bar, 12);
    gtk_widget_set_margin_end(input_bar, 12);
    gtk_widget_set_margin_top(input_bar, 8);
    gtk_widget_set_margin_bottom(input_bar, 12);

    attach_btn_ = gtk_button_new_from_icon_name("mail-attachment-symbolic");
    gtk_widget_add_css_class(attach_btn_, "flat");
    gtk_widget_add_css_class(attach_btn_, "circular");
    gtk_widget_set_sensitive(attach_btn_, FALSE);
    gtk_widget_set_tooltip_text(attach_btn_, "Attach files");
    g_signal_connect_swapped(attach_btn_, "clicked",
        G_CALLBACK(+[](MainWindow* self) { self->pick_files(); }), this);

    text_entry_ = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(text_entry_), "Message…");
    gtk_widget_set_hexpand(text_entry_, TRUE);
    gtk_widget_set_sensitive(text_entry_, FALSE);
    g_signal_connect(text_entry_, "activate",
        G_CALLBACK(+[](GtkEntry*, gpointer ud) {
            auto* self = static_cast<MainWindow*>(ud);
            const char* t = gtk_editable_get_text(GTK_EDITABLE(self->text_entry_));
            if (t && *t && self->app_.active_conn()) {
                self->app_.send_text(t);
                gtk_editable_set_text(GTK_EDITABLE(self->text_entry_), "");
            }
        }), this);

    send_btn_ = gtk_button_new_from_icon_name("go-up-symbolic");
    gtk_widget_add_css_class(send_btn_, "suggested-action");
    gtk_widget_add_css_class(send_btn_, "circular");
    gtk_widget_set_sensitive(send_btn_, FALSE);
    gtk_widget_set_tooltip_text(send_btn_, "Send");
    g_signal_connect_swapped(send_btn_, "clicked",
        G_CALLBACK(+[](MainWindow* self) {
            const char* t = gtk_editable_get_text(GTK_EDITABLE(self->text_entry_));
            if (t && *t) {
                self->app_.send_text(t);
                gtk_editable_set_text(GTK_EDITABLE(self->text_entry_), "");
            }
        }), this);

    gtk_box_append(GTK_BOX(input_bar), attach_btn_);
    gtk_box_append(GTK_BOX(input_bar), text_entry_);
    gtk_box_append(GTK_BOX(input_bar), send_btn_);
    gtk_box_append(GTK_BOX(transfer_content_), input_bar);
}

void MainWindow::refresh_conn_state(bool connected) {
    gtk_widget_set_visible(disconnect_btn_, connected);
    gtk_widget_set_sensitive(send_btn_, connected);
    gtk_widget_set_sensitive(attach_btn_, connected);
    gtk_widget_set_sensitive(text_entry_, connected);
}

void MainWindow::refresh_transfers(const std::vector<TransferItem>& items) {
    std::set<std::string> seen;
    for (auto& item : items) seen.insert(item.id);

    for (auto it = transfer_rows_.begin(); it != transfer_rows_.end(); ) {
        if (!seen.count(it->first)) {
            gtk_list_box_remove(GTK_LIST_BOX(transfers_list_), it->second);
            row_widgets_.erase(it->first);
            it = transfer_rows_.erase(it);
        } else { ++it; }
    }

    int position = 0;
    for (auto& item : items) {
        auto rit = transfer_rows_.find(item.id);
        if (rit == transfer_rows_.end()) {
            GtkWidget* row = gtk_list_box_row_new();
            gtk_list_box_row_set_activatable(GTK_LIST_BOX_ROW(row), FALSE);

            GtkWidget* outer = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
            gtk_widget_set_margin_start(outer, 10);
            gtk_widget_set_margin_end(outer, 10);
            gtk_widget_set_margin_top(outer, 8);
            gtk_widget_set_margin_bottom(outer, 8);

            const char* kind_icon = "chat-symbolic";
            if (item.kind == TransferItem::Kind::File)   kind_icon = "document-symbolic";
            if (item.kind == TransferItem::Kind::Logcat) kind_icon = "utilities-terminal-symbolic";
            GtkWidget* icon = gtk_image_new_from_icon_name(kind_icon);
            gtk_image_set_pixel_size(GTK_IMAGE(icon), 16);
            gtk_widget_add_css_class(icon,
                item.dir == TransferDir::In ? "accent" : "dim-label");
            gtk_widget_set_valign(icon, GTK_ALIGN_START);
            gtk_widget_set_margin_top(icon, 2);
            gtk_box_append(GTK_BOX(outer), icon);

            GtkWidget* box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 3);
            gtk_widget_set_hexpand(box, TRUE);

            std::string who = (item.dir == TransferDir::In)
                ? item.device_name : "You";
            GtkWidget* who_l = make_label(who.c_str(), "caption");
            gtk_widget_add_css_class(who_l,
                item.dir == TransferDir::In ? "accent" : "dim-label");
            gtk_box_append(GTK_BOX(box), who_l);

            RowWidgets rw{};

            if (item.kind == TransferItem::Kind::Text) {
                struct CopyCtx { AdwToastOverlay* overlay; std::string text; };

                GtkWidget* row_h = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
                GtkWidget* txt = gtk_label_new(item.content.c_str());
                gtk_label_set_selectable(GTK_LABEL(txt), TRUE);
                gtk_label_set_wrap(GTK_LABEL(txt), TRUE);
                gtk_label_set_xalign(GTK_LABEL(txt), 0);
                gtk_widget_set_hexpand(txt, TRUE);

                GtkWidget* copy = gtk_button_new_from_icon_name("edit-copy-symbolic");
                gtk_widget_add_css_class(copy, "flat");
                gtk_widget_add_css_class(copy, "circular");
                gtk_widget_set_valign(copy, GTK_ALIGN_START);
                g_signal_connect_data(copy, "clicked",
                    G_CALLBACK(+[](GtkButton*, gpointer ud) {
                        auto* c = static_cast<CopyCtx*>(ud);
                        gdk_clipboard_set_text(
                            gdk_display_get_clipboard(gdk_display_get_default()),
                            c->text.c_str());
                        adw_toast_overlay_add_toast(c->overlay,
                            adw_toast_new("Copied"));
                    }),
                    new CopyCtx{toast_overlay_, item.content},
                    [](gpointer d, GClosure*) { delete static_cast<CopyCtx*>(d); },
                    G_CONNECT_DEFAULT);

                gtk_box_append(GTK_BOX(row_h), txt);
                gtk_box_append(GTK_BOX(row_h), copy);
                gtk_box_append(GTK_BOX(box), row_h);

            } else if (item.kind == TransferItem::Kind::File) {
                GtkWidget* name_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
                GtkWidget* name_l = make_label(item.file_name.c_str());
                gtk_widget_set_hexpand(name_l, TRUE);
                GtkWidget* size_l = make_label(fmt_size(item.file_size).c_str(), "dim-label");
                gtk_widget_add_css_class(size_l, "caption");
                gtk_widget_set_valign(size_l, GTK_ALIGN_CENTER);
                gtk_box_append(GTK_BOX(name_row), name_l);
                gtk_box_append(GTK_BOX(name_row), size_l);
                gtk_box_append(GTK_BOX(box), name_row);

                rw.progress = gtk_progress_bar_new();
                gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(rw.progress), item.progress);
                gtk_widget_set_visible(rw.progress, item.progress < 1.f);
                gtk_box_append(GTK_BOX(box), rw.progress);

                rw.sub_label = make_label("", "dim-label");
                gtk_widget_add_css_class(rw.sub_label, "caption");
                gtk_box_append(GTK_BOX(box), rw.sub_label);

                rw.open_btn = gtk_button_new_from_icon_name("folder-open-symbolic");
                gtk_widget_add_css_class(rw.open_btn, "flat");
                gtk_widget_add_css_class(rw.open_btn, "circular");
                gtk_widget_set_halign(rw.open_btn, GTK_ALIGN_START);
                gtk_widget_set_tooltip_text(rw.open_btn, "Open file");
                gtk_widget_set_visible(rw.open_btn, FALSE);
                g_signal_connect_data(rw.open_btn, "clicked",
                    G_CALLBACK(+[](GtkButton*, gpointer ud) {
                        auto* path = static_cast<std::string*>(ud);
                        GFile* f = g_file_new_for_path(path->c_str());
                        GtkUriLauncher* launcher = gtk_uri_launcher_new(g_file_get_uri(f));
                        gtk_uri_launcher_launch(launcher, nullptr, nullptr, nullptr, nullptr);
                        g_object_unref(launcher);
                        g_object_unref(f);
                    }),
                    new std::string(item.local_path),
                    [](gpointer d, GClosure*) { delete static_cast<std::string*>(d); },
                    G_CONNECT_DEFAULT);
                gtk_box_append(GTK_BOX(box), rw.open_btn);

            } else {
                std::string head = "Logcat — " + std::to_string(item.log_lines.size()) + " lines";
                rw.sub_label = make_label(head.c_str(), "dim-label");
                gtk_widget_add_css_class(rw.sub_label, "caption");
                gtk_box_append(GTK_BOX(box), rw.sub_label);

                GtkWidget* tail = gtk_box_new(GTK_ORIENTATION_VERTICAL, 1);
                gtk_widget_add_css_class(tail, "card");
                gtk_widget_set_margin_top(tail, 4);
                size_t start = item.log_lines.size() > 4
                    ? item.log_lines.size() - 4 : 0;
                for (size_t i = start; i < item.log_lines.size(); i++) {
                    GtkWidget* ll = make_label(item.log_lines[i].c_str(), "dim-label");
                    gtk_widget_add_css_class(ll, "monospace");
                    gtk_widget_add_css_class(ll, "caption");
                    gtk_label_set_wrap(GTK_LABEL(ll), TRUE);
                    gtk_box_append(GTK_BOX(tail), ll);
                }
                gtk_box_append(GTK_BOX(box), tail);
            }

            gtk_box_append(GTK_BOX(outer), box);
            gtk_list_box_row_set_child(GTK_LIST_BOX_ROW(row), outer);
            gtk_list_box_insert(GTK_LIST_BOX(transfers_list_), row, position);
            transfer_rows_[item.id] = row;
            row_widgets_[item.id]   = rw;

        } else {
            auto& rw = row_widgets_[item.id];
            if (item.kind == TransferItem::Kind::File) {
                if (rw.progress) {
                    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(rw.progress), item.progress);
                    gtk_widget_set_visible(rw.progress, item.progress < 1.f);
                }
                if (rw.sub_label) {
                    if (item.progress < 1.f)
                        gtk_label_set_text(GTK_LABEL(rw.sub_label),
                            (std::to_string(item.recv_chunks) + " / " +
                             std::to_string(item.total_chunks) + " chunks").c_str());
                    else
                        gtk_label_set_text(GTK_LABEL(rw.sub_label), item.local_path.c_str());
                }
                if (rw.open_btn && item.progress >= 1.f && !item.local_path.empty())
                    gtk_widget_set_visible(rw.open_btn, TRUE);
            } else if (item.kind == TransferItem::Kind::Logcat && rw.sub_label) {
                gtk_label_set_text(GTK_LABEL(rw.sub_label),
                    ("Logcat — " + std::to_string(item.log_lines.size()) + " lines").c_str());
            }
        }
        ++position;
    }
}

void MainWindow::pick_files() {
    GtkFileDialog* fd = gtk_file_dialog_new();
    gtk_file_dialog_set_title(fd, "Select files to send");
    gtk_file_dialog_open_multiple(fd, GTK_WINDOW(window_), nullptr,
        [](GObject* src, GAsyncResult* res, gpointer ud) {
            auto* self = static_cast<MainWindow*>(ud);
            GListModel* files = gtk_file_dialog_open_multiple_finish(
                GTK_FILE_DIALOG(src), res, nullptr);
            if (!files) { g_object_unref(src); return; }
            guint n = g_list_model_get_n_items(files);
            for (guint i = 0; i < n; i++) {
                auto* file = G_FILE(g_list_model_get_item(files, i));
                char* path = g_file_get_path(file);
                if (path) { self->app_.send_file(path); g_free(path); }
                g_object_unref(file);
            }
            g_object_unref(files);
            g_object_unref(src);
        }, this);
}

void MainWindow::show_settings() {
    const AppSettings& s = app_.settings();

    AdwPreferencesDialog* dlg =
        ADW_PREFERENCES_DIALOG(adw_preferences_dialog_new());
    adw_preferences_dialog_set_search_enabled(dlg, FALSE);

    AdwPreferencesPage* page = ADW_PREFERENCES_PAGE(adw_preferences_page_new());
    adw_preferences_dialog_add(dlg, page);

    AdwPreferencesGroup* dev_grp = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
    adw_preferences_group_set_title(dev_grp, "Device");
    adw_preferences_page_add(page, dev_grp);

    AdwEntryRow* name_row = ADW_ENTRY_ROW(adw_entry_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(name_row), "Name");
    gtk_editable_set_text(GTK_EDITABLE(name_row), s.device_name.c_str());
    adw_preferences_group_add(dev_grp, GTK_WIDGET(name_row));

    AdwPreferencesGroup* net_grp = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
    adw_preferences_group_set_title(net_grp, "Network");
    adw_preferences_group_set_description(net_grp, "Port change takes effect on next launch");
    adw_preferences_page_add(page, net_grp);

    AdwSpinRow* port_row = ADW_SPIN_ROW(adw_spin_row_new_with_range(1024, 65535, 1));
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(port_row), "Port");
    adw_spin_row_set_value(port_row, s.port);
    adw_preferences_group_add(net_grp, GTK_WIDGET(port_row));

    AdwPreferencesGroup* xfer_grp = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
    adw_preferences_group_set_title(xfer_grp, "Transfer");
    adw_preferences_group_set_description(xfer_grp,
        "Leave download directory empty to use ~/Downloads/fly");
    adw_preferences_page_add(page, xfer_grp);

    AdwEntryRow* dir_row = ADW_ENTRY_ROW(adw_entry_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(dir_row), "Download Directory");
    gtk_editable_set_text(GTK_EDITABLE(dir_row), s.download_dir.c_str());
    adw_preferences_group_add(xfer_grp, GTK_WIDGET(dir_row));

    AdwSpinRow* thresh_row = ADW_SPIN_ROW(adw_spin_row_new_with_range(16, 512, 1));
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(thresh_row), "Chunk Threshold (MB)");
    adw_action_row_set_subtitle(ADW_ACTION_ROW(thresh_row),
        "Files larger than this are sent in chunks");
    adw_spin_row_set_value(thresh_row, s.chunk_threshold_mb);
    adw_preferences_group_add(xfer_grp, GTK_WIDGET(thresh_row));

    AdwSpinRow* chunk_row = ADW_SPIN_ROW(adw_spin_row_new_with_range(1, 32, 1));
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(chunk_row), "Chunk Size (MB)");
    adw_spin_row_set_value(chunk_row, s.chunk_size_mb);
    adw_preferences_group_add(xfer_grp, GTK_WIDGET(chunk_row));

    AdwPreferencesGroup* app_grp = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
    adw_preferences_group_set_title(app_grp, "Appearance");
    adw_preferences_page_add(page, app_grp);

    const char* theme_opts[] = {"System", "Dark", "Light", nullptr};
    AdwComboRow* theme_row = ADW_COMBO_ROW(adw_combo_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(theme_row), "Color Scheme");
    adw_combo_row_set_model(theme_row,
        G_LIST_MODEL(gtk_string_list_new(theme_opts)));
    int tidx = (s.theme_mode == "dark") ? 1 : (s.theme_mode == "light") ? 2 : 0;
    adw_combo_row_set_selected(theme_row, (guint)tidx);
    adw_preferences_group_add(app_grp, GTK_WIDGET(theme_row));

    auto theme_changed = +[](AdwComboRow* row, GParamSpec*, gpointer) {
        const char* tkeys[3] = {"system", "dark", "light"};
        apply_theme(tkeys[adw_combo_row_get_selected(row)]);
    };
    g_signal_connect(theme_row, "notify::selected", G_CALLBACK(theme_changed), nullptr);

    AdwPreferencesGroup* reset_grp = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
    adw_preferences_page_add(page, reset_grp);
    AdwButtonRow* reset_row = ADW_BUTTON_ROW(adw_button_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(reset_row), "Reset to Defaults");
    gtk_widget_add_css_class(GTK_WIDGET(reset_row), "destructive-action");
    adw_preferences_group_add(reset_grp, GTK_WIDGET(reset_row));

    struct Ctx {
        MainWindow*           win;
        AdwPreferencesDialog* dlg;
        AdwEntryRow* name; AdwSpinRow* port;
        AdwEntryRow* dir;  AdwSpinRow* thresh; AdwSpinRow* chunk;
        AdwComboRow* theme;
    };
    auto* ctx = new Ctx{this, dlg, name_row, port_row, dir_row,
                        thresh_row, chunk_row, theme_row};

    auto reset_fn = +[](AdwButtonRow*, gpointer ud) {
        auto* c = static_cast<Ctx*>(ud);
        AppSettings def;
        gtk_editable_set_text(GTK_EDITABLE(c->name), def.device_name.c_str());
        adw_spin_row_set_value(c->port, def.port);
        gtk_editable_set_text(GTK_EDITABLE(c->dir), def.download_dir.c_str());
        adw_spin_row_set_value(c->thresh, def.chunk_threshold_mb);
        adw_spin_row_set_value(c->chunk, def.chunk_size_mb);
        adw_combo_row_set_selected(c->theme, 0);
        adw_dialog_close(ADW_DIALOG(c->dlg));
    };
    g_signal_connect(reset_row, "activated", G_CALLBACK(reset_fn), ctx);

    auto save_fn = +[](AdwPreferencesDialog*, gpointer ud) -> gboolean {
        auto* c = static_cast<Ctx*>(ud);
        AppSettings ns;
        ns.device_name        = gtk_editable_get_text(GTK_EDITABLE(c->name));
        ns.port               = (int)adw_spin_row_get_value(c->port);
        ns.download_dir       = gtk_editable_get_text(GTK_EDITABLE(c->dir));
        ns.chunk_threshold_mb = (int)adw_spin_row_get_value(c->thresh);
        ns.chunk_size_mb      = (int)adw_spin_row_get_value(c->chunk);
        const char* tkeys[3]  = {"system", "dark", "light"};
        ns.theme_mode         = tkeys[adw_combo_row_get_selected(c->theme)];
        c->win->app_.save_settings(ns);
        delete c;
        return FALSE;
    };
    g_signal_connect(dlg, "close-request", G_CALLBACK(save_fn), ctx);

    adw_dialog_present(ADW_DIALOG(dlg), GTK_WIDGET(window_));
}
