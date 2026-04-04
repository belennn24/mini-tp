#include <utils.h>

t_log* logger;
t_log_level log_level;
t_config* config;

int main(int argc, char* argv[]) {
    saludar("memoria");
    logger = log_create("memoria.log", "MEMORIA", 0, LOG_LEVEL_INFO);
    iniciar_config();
    return 0;
}

void iniciar_config(){
    config = config_create("memoria.config");
    log_level = log_level_from_string(config_get_string_value(config, "LOG_LEVEL"));

}