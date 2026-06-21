project('fly', 'cpp',
  version: '1.0.0',
  default_options: ['cpp_std=c++20', 'warning_level=2'])

gtk4_dep        = dependency('gtk4')
adwaita_dep     = dependency('libadwaita-1')
json_dep        = dependency('nlohmann_json',
                    fallback: ['nlohmann_json', 'nlohmann_json_dep'])

src = files(
  'src/main.cpp',
  'src/app.cpp',
  'src/protocol/fly_frame.cpp',
  'src/protocol/fly_message.cpp',
  'src/network/peer_connection.cpp',
  'src/network/fly_server.cpp',
  'src/network/fly_client.cpp',
  'src/discovery/device_discovery.cpp',
  'src/transfer/transfer_manager.cpp',
  'src/ui/main_window.cpp',
)

executable('fly', src,
  dependencies: [gtk4_dep, adwaita_dep, json_dep],
  install: true)

install_data('data/fly.desktop',
  install_dir: get_option('datadir') / 'applications')
