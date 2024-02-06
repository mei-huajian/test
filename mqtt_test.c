#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <pthread.h>
#include "mqtt_config.h"
#include "mqtt_log.h"
#include "mqttclient.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>

// #define TEST_USEING_TLS  
// extern const char *test_ca_get();
int fd;

static void topic1_handler(void* client, message_data_t* msg)
{
    (void) client;
    MQTT_LOG_I("-----------------------------------------------------------------------------------");
    MQTT_LOG_I("%s:%d %s()...\ntopic: %s\nmessage:%s", __FILE__, __LINE__, __FUNCTION__, msg->topic_name, (char*)msg->message->payload);
    MQTT_LOG_I("-----------------------------------------------------------------------------------");
}

void *mqtt_publish_thread(void *arg)
{
    mqtt_client_t *client = (mqtt_client_t *)arg;
	int count=0;

    char buf[100] = { 0 };
    mqtt_message_t msg;
    memset(&msg, 0, sizeof(msg));

    sleep(2);

    mqtt_list_subscribe_topic(client);

    msg.payload = (void *) buf;

    while(1) {
        if(read(fd, buf, sizeof(buf)) > 0)
		{
			msg.qos = 0;
            mqtt_publish(client, "arm-linux", &msg);
			memset(buf, 0, sizeof(buf));
		}
    }
}

int main(void)
{
    int res;
    pthread_t thread1;
    mqtt_client_t *client = NULL;

    //创建有名管道
	if(access("fifo", F_OK) == -1)
	{
		if(mkfifo("fifo", 0666) == -1)
		{
			printf("Cannot create fifo file\n");
		}
		printf("create fifo is success!\n");
	}
	fd = open("fifo", O_RDONLY);
    
    mqtt_log_init();

    client = mqtt_lease();

#ifdef TEST_USEING_TLS
    mqtt_set_port(client, "8883");
    mqtt_set_ca(client, (char*)test_ca_get());
#else
    mqtt_set_port(client, "1883");
#endif

	mqtt_set_host(client, "192.168.137.1");
    mqtt_set_client_id(client, random_string(10));
    mqtt_set_user_name(client, random_string(10));
    mqtt_set_password(client, random_string(10));
    mqtt_set_clean_session(client, 1);

    mqtt_connect(client);
    
    mqtt_subscribe(client, "topic1", QOS0, topic1_handler);
    
    res = pthread_create(&thread1, NULL, mqtt_publish_thread, client);
    if(res != 0) {
        MQTT_LOG_E("create mqtt publish thread fail");
        exit(res);
    }

    while (1) {
        sleep(100);
    }
}
