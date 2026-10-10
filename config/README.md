# LmControl middleware configuration

These checked-in JSON documents preserve three distinct deployment configurations.
They share a service type definition but intentionally differ in instance settings.

| JSON source | Profile | Effective settings |
| --- | --- | --- |
| `mw_com_config.json` | Integration | Both LaunchManager/StateManager and StateManager/LaunchManager instances, QM |
| `mw_com_config_provider_test.json` | Provider test | LaunchManager/StateManager instance, QM, eight sample slots and one subscriber |
| `mw_com_config_client_test.json` | Client test | StateManager/LaunchManager instance, ASIL-B, strict permission checks, provider UID 3020 and B-receiver queue size 10 |

The public target `//score/launch_manager:mw_com_config` exports the integration
document as a starting point. Review the instance specifiers, ASIL level, provider
permissions and queue sizes before adapting it for another deployment. This target
does not select a configuration based on the consumer or provide a production default.
The explicitly named test profiles remain separately available from `//config`.

The existing test output labels and runtime filenames are retained through copies
of these sources. No JSON fields are generated or combined at build time.

The JSON bytes are copied from the preexisting Apache-2.0 configuration/profile data.
Adjacent `.license` files retain its copyright, NOTICE reference and license terms.
