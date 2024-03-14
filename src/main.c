/*
 * Copyright (c) 2023 Golioth, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(golioth_temperature, LOG_LEVEL_DBG);

#include <golioth/client.h>
#include <golioth/stream.h>
#include <samples/common/net_connect.h>
#include <samples/common/sample_credentials.h>

#include <zephyr/drivers/sensor.h>
#include <stdlib.h>

#define JSON_FMT "{\"hum\":\"%.6f\",\"pre\":\"%.6f\",\"tem\":\"%.6f\"}"
static struct golioth_client *client;
static K_SEM_DEFINE(connected, 0, 1);

static void on_client_event(struct golioth_client *client,
			    enum golioth_client_event event,
			    void *arg)
{
	bool is_connected = (event == GOLIOTH_CLIENT_EVENT_CONNECTED);

	if (is_connected) {
		k_sem_give(&connected);
	}
	LOG_INF("Golioth client %s", is_connected ? "connected" : "disconnected");
}

static void async_handler(struct golioth_client *client,
				const struct golioth_response *response,
				const char *path,
				void *arg)
{
	if (response->status != GOLIOTH_OK) {
		LOG_ERR("Failed to send temperature to Golioth: %d", response->status);
		return;
	}
}

int main(void)
{
	int err;
	struct sensor_value hum, pre, tem;
	char sbuf[64];

	if (IS_ENABLED(CONFIG_GOLIOTH_SAMPLE_COMMON)) {
		net_connect();
	}

	const struct golioth_client_config *client_config = golioth_sample_credentials_get();
	client = golioth_client_create(client_config);
	golioth_client_register_event_callback(client, on_client_event, NULL);

	k_sem_take(&connected, K_FOREVER);

	const struct device *weather_dev = DEVICE_DT_GET(DT_ALIAS(weather));

	if (weather_dev == NULL) {
		LOG_ERR("Error: no sensor found.");
	}

	if (!device_is_ready(weather_dev)) {
		LOG_ERR("Error: Sensor %s is not ready;"
			"check the driver initialization logs for errors.",
			weather_dev->name);
	} else {
		LOG_INF("Found sensor: %s", weather_dev->name);
	}

	while (true) {
		sensor_sample_fetch(weather_dev);
		sensor_channel_get(weather_dev, SENSOR_CHAN_HUMIDITY, &hum);
		sensor_channel_get(weather_dev, SENSOR_CHAN_PRESS, &pre);
		sensor_channel_get(weather_dev, SENSOR_CHAN_AMBIENT_TEMP, &tem);

		snprintk(sbuf,
		         sizeof(sbuf),
			 JSON_FMT,
			 sensor_value_to_double(&hum),
			 sensor_value_to_double(&pre),
			 sensor_value_to_double(&tem)
			);
		LOG_DBG("Sending temperature %s", sbuf);

		err = golioth_stream_set_async(client,
					      "env",
					      GOLIOTH_CONTENT_TYPE_JSON,
					      sbuf,
					      strlen(sbuf),
					      async_handler,
					      NULL);

		if (err) {
			LOG_WRN("Failed to stream temperature: %d", err);
		}

		k_sleep(K_SECONDS(5));
	}
}
