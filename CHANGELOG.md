# CHANGELOG

## [0.8.10](https://github.com/uhppoted/uhppoted-wiegand/releases/tag/v0.8.10) - 2025-01-31

### Updated
1. Updated labels in READ.pio for RP2350 reserved words.


## [0.8.9](https://github.com/uhppoted/uhppoted-wiegand/releases/tag/v0.8.9) - 2024-09-06

1. Maintenance release for version consistency.


## [0.8.8](https://github.com/uhppoted/uhppoted-wiegand/releases/tag/v0.8.8) - 2024-03-27

1. Maintenance release for version consistency.


## [0.8.7](https://github.com/uhppoted/uhppoted-wiegand/releases/tag/v0.8.7) - 2023-12-01

### Added
1. Keypad emulation
2. Passcode access override
3. Card + optional PIN access
4. _clear-acl_ command

### Updated
1. Fixed bug in SD card _store-acl_ logic.
2. Renamed 'reference' implementation to 'universal'


## [0.8.6](https://github.com/uhppoted/uhppoted-wiegand/releases/tag/v0.8.6) - 2023-08-30

### Added
1. Uses on-board flash for ACL

### Updated
2. Fixed SD card ACL write for missing ACL file.


## [0.8.5](https://github.com/uhppoted/uhppoted-wiegand/releases/tag/v0.8.5) - 2023-06-14

### Added
1. PicoW base and USB variants
2. PicoW WiFi variants


## [0.8.4](https://github.com/uhppoted/uhppoted-wiegand/releases/tag/v0.8.4) - 2023-03-17

1. Initial release of Raspberry Pi Pico Wiegand-26 reader/emulator

## Older

| *Version* | *Description*                                                                                               |
| --------- | ----------------------------------------------------------------------------------------------------------- |
| v0.8.3    | Adds AMR64 to release builds                                                                                |
| v0.8.2    | Adds _uhppoted-codegen_ project to generate native UHPPOTE interface in languages other thann Go            |
| v0.8.1    | Adds human readable fields to events for MQTT and REST implementations. Minor bug fixes and usability fixes |
| v0.8.0    | Initial release for _uhppoted-httpd_ and _uhppoted-tunnel_                                                  |
| v0.7.3    | Adds [uhppoted-dll](https://github.com/uhppoted/uhppoted-dll) shared-lib to components                      |
| v0.7.2    | Replaces event rollover with _forever_ incrementing indices to match observed behaviour                     |
| v0.7.1    | Adds support for controller task list management                                                            |
| v0.7.0    | Adds support for time profiles. Includes bundled release of `uhppoted-nodejs` NodeJS module                 |
| v0.6.12   | Reworked concurrency and additional configuration                                                           |
| v0.6.10   | Initial release for `uhppoted-app-wild-apricot`                                                             |
| v0.6.8    | Adds handling for v6.62 firmware _listen events_ to `node-red-contrib-uhppoted`                             |
| v0.6.7    | Implements `record-special-events` to enable/disable door events                                            |
| v0.6.5    | `node-red-contrib-uhppoted` module for use with NodeRED low code environment                                |
| v0.6.4    | `uhppoted-app-sheets` Google Sheets integration module                                                      |
| v0.6.3    | Added access control list commands to `uhppoted-mqtt`                                                       |
| v0.6.2    | Added access control list commands to `uhppoted-rest`                                                       |
| v0.6.1    | Added access control list commands to `uhppote-cli`                                                         |
| v0.6.0    | `uhppoted-app-s3` AWS S3 integration module                                                                 |
| v0.5.1    | Initial release following restructuring into standalone Go *modules* and *git submodules*                   |
| v0.5.0    | Add MQTT endpoint for remote access to UT0311-L0x controllers                                               |
| v0.4.2    | Reworked `GetDevice` REST API to use directed broadcast and added get-device to CLI                         |
| v0.4.1    | Get/set door control state functionality added to simulator, CLI and REST API                               |
| v0.4.0    | REST API service                                                                                            |
| v0.3.1    | Functional simulator with minimal command API                                                               |
| v0.2.0    | Load access control list from TSV file                                                                      |
| v0.1.0    | Bare-bones but functional CLI                                                                               |
