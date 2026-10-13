// API: console, logger, unity, Vector3
console.log("QuickJS script started");
console.warn("Example warning");
console.error("Example error output");
logger.info("Message from the native logger binding");

const domain = mono.get_root_domain();
const player = unity.get_local_player();
const zones = unity.get_zone_system();
const environment = unity.get_env_man();
console.log("Mono domain:", domain);
console.log("Player / ZoneSystem / EnvMan:", player, zones, environment);

const position = new Vector3(12.5, 45, -9.8);
console.log("Vector3:", position.x, position.y, position.z);

