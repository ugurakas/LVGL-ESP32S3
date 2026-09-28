#include "config_store.h"
#include "sensor_model.h"
#include <zephyr/shell/shell.h>
#include <zephyr/sys/reboot.h>
static int set_config(const struct shell *sh, size_t argc, char **argv)
{
    ARG_UNUSED(argc);
    int err = config_store_set(argv[1], argv[2]);
    if (err) {
        shell_error(sh, "Setting rejected: %d", err);
    } else {
        shell_print(sh, "Saved. Reboot to apply.");
    }
    return err;
}
static int refresh(const struct shell *sh, size_t argc, char **argv)
{
    ARG_UNUSED(sh); ARG_UNUSED(argc); ARG_UNUSED(argv);
    sensor_request_refresh();
    return 0;
}
static int reboot_device(const struct shell *sh, size_t argc, char **argv)
{
    ARG_UNUSED(sh); ARG_UNUSED(argc); ARG_UNUSED(argv);
    sys_reboot(SYS_REBOOT_COLD);
    return 0;
}
SHELL_STATIC_SUBCMD_SET_CREATE(panel_commands,
    SHELL_CMD_ARG(set, NULL, "set <ssid|password|api_host|api_path|api_body|ntp_host> <value>",
                  set_config, 3, 0),
    SHELL_CMD_ARG(refresh, NULL, "Request a fresh sample", refresh, 1, 0),
    SHELL_CMD_ARG(reboot, NULL, "Reboot to apply settings", reboot_device, 1, 0),
    SHELL_SUBCMD_SET_END
);
SHELL_CMD_REGISTER(panel, &panel_commands, "Panel provisioning", NULL);
