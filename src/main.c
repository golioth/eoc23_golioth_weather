/*
 * Copyright (c) 2023 Golioth, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(golioth_temperature, LOG_LEVEL_DBG);

#include <net/golioth/system_client.h>
#include <samples/common/net_connect.h>
#include <zephyr/net/coap.h>

#include <zephyr/drivers/sensor.h>
#include <stdlib.h>

#define SENSOR_JSON_FMT "{\"tem\":%d.%d,\"pre\":%d.%d,\"hum\":%d.%d}"

static struct golioth_client *client = GOLIOTH_SYSTEM_CLIENT_GET();
static K_SEM_DEFINE(connected, 0, 1);

static void golioth_on_connect(struct golioth_client *client)
{
	k_sem_give(&connected);
}

static int async_handler(struct golioth_req_rsp *rsp) {
	if (rsp->err) {
		LOG_WRN("Failed to push temperature: %d", rsp->err);
		return rsp->err;
	}

	return 0;
}

void main(void)
{
	int err;
	struct sensor_value tem;
	struct sensor_value pre;
	struct sensor_value hum;
	char sbuf[128];

	if (IS_ENABLED(CONFIG_GOLIOTH_SAMPLES_COMMON)) {
		net_connect();
	}

	client->on_connect = golioth_on_connect;
	golioth_system_client_start();

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

	int counter = 0;

	while (true) {
		sensor_sample_fetch(weather_dev);
		sensor_channel_get(weather_dev, SENSOR_CHAN_AMBIENT_TEMP, &tem);
		sensor_channel_get(weather_dev, SENSOR_CHAN_PRESS, &pre);
		sensor_channel_get(weather_dev, SENSOR_CHAN_HUMIDITY, &hum);

		snprintk(sbuf, sizeof(sbuf),
			 SENSOR_JSON_FMT,
			 tem.val1, abs(tem.val2),
			 pre.val1, abs(pre.val2),
			 hum.val1, abs(hum.val2)
			 );

		LOG_INF("Sending sensor reading #%d", counter);
		++counter;

		err = golioth_stream_push_cb(client, "weather",
					     GOLIOTH_CONTENT_FORMAT_APP_JSON,
					     sbuf, strlen(sbuf),
					     async_handler, NULL);

		if (err) {
			LOG_WRN("Failed to push weather: %d", err);
			return;
		}

		k_sleep(K_SECONDS(5));
	}
}
