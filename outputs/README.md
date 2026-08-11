# Firmware artifacts

`red_monkey_mpg_production_receiver.uf2` is the only unified receiver release
candidate. It is still development-identified and unsigned; do not sell or
represent it as a commercial release.

Every other UF2 in this directory is a diagnostic, preview, or commissioning
image. Some deliberately emit keyboard jog commands. Never package or ship
those images as normal receiver firmware. Build commercial release bundles in
a separate, clean staging directory containing only the reviewed receiver UF2,
SBOM, signed manifest/checksum, license, and user/safety instructions.
