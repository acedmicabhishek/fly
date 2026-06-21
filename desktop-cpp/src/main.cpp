#include "app.h"
#include "ui/main_window.h"
#include <adwaita.h>
#include <memory>

struct ActivateCtx {
    App* app;
    std::unique_ptr<MainWindow>* win;
};

static void on_activate(AdwApplication* adw_app, gpointer user_data) {
    auto* ctx = static_cast<ActivateCtx*>(user_data);
    ctx->app->start();
    ctx->win->reset(new MainWindow(*ctx->app, adw_app));
    gtk_window_present(GTK_WINDOW((*ctx->win)->widget()));
}

int main(int argc, char* argv[]) {
    AdwApplication* adw_app = adw_application_new(
        "com.fly.desktop", G_APPLICATION_DEFAULT_FLAGS);

    App app;
    std::unique_ptr<MainWindow> win;
    ActivateCtx ctx{&app, &win};

    g_signal_connect(adw_app, "activate", G_CALLBACK(on_activate), &ctx);

    int status = g_application_run(G_APPLICATION(adw_app), argc, argv);
    g_object_unref(adw_app);
    return status;
}
