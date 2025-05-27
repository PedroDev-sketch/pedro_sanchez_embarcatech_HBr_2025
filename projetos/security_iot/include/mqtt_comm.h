static void mqtt_connection_cb(mqtt_client_t *client, void *arg, mqtt_connection_status_t status);

void mqtt_setup(const char *client_id, const char *broker_ip, const char *user, const char *pass);

static void mqtt_pub_request_cb(void *arg, err_t result);

void mqtt_comm_publish(const char *topic, const uint8_t *data, size_t len);

static void mqtt_incoming_publish_cb(void *arg, const char *topic, u32_t tot_len);

static void mqtt_incoming_data_cb(void *arg, const u8_t *data, u16_t len, u8_t flags);
