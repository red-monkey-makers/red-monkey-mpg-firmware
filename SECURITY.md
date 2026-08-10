# Security policy

## Supported versions

No Red Monkey MPG version is currently supported as a commercial or safety-rated
product. `0.3.2-production-rc1` is a test release candidate. Do not report it as
certified, safety-rated, or approved for unattended operation.

## Reporting a vulnerability

**Report privately through
[GitHub Security Advisories](https://github.com/red-monkey-makers/red-monkey-mpg-firmware/security/advisories/new)**
("Report a vulnerability" on the repository Security tab). Do not open a public
issue for a suspected vulnerability.

Do not include controller Bluetooth addresses, receiver serial numbers,
manufacturing keys, or identifying machine details in any report — public or
private.

Response targets, best effort for a pre-commercial project:

- Acknowledgement within 5 business days.
- An initial assessment, including whether motion safety is affected, within
  15 business days.
- Coordinated disclosure once a fix or documented mitigation is available.

Before commercial release the maintainer must additionally publish a
supported-version policy and a monitored disclosure address; commercial release
remains blocked until both exist.

## Security boundaries

- Red Monkey MPG is not an E-stop, protective device, or safety PLC.
- The hard-wired machine E-stop and normal safeguards are outside and above the
  Red Monkey MPG trust boundary.
- Physical USB access plus explicit browser Web Serial permission authorizes
  configuration. The configurator does not provide user authentication.
- Bluetooth Classic Just Works pairing is not resistant to an active radio
  attacker by itself. Initial binding also requires a physical three-second
  controller chord, and reconnects require the stored link key.
- Only one compiled controller report profile is accepted. User configuration
  cannot create raw keys, macros, multi-axis motion, or remove dead-man and
  neutral-rearm invariants.

## Release security requirements

Every commercial release must use an authorized USB VID/PID, an independently
reviewed signed-boot/key-provisioning process, reproducible artifacts with an
SBOM and signed checksums, a tested update/recovery policy, dependency review,
and external security and machine-safety review. See
[`docs/PRODUCTION_READINESS.md`](docs/PRODUCTION_READINESS.md).
