/* Sockets Example
 * Copyright (c) 2016-2020 ARM Limited
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "mbed.h"
#include "math.h"
#include "arm_math.h"
#include "wifi_helper.h"
#include "mbed-trace/mbed_trace.h"
#include "stm32l475e_iot01.h"
#include "stm32l475e_iot01_accelero.h"
#include "stm32l475e_iot01_gyro.h"
#include <cmath>
#include <cstdint>
#include <cstdio>

#if defined(SEMIHOSTING)
#include <stdio.h>
#endif


#if MBED_CONF_APP_USE_TLS_SOCKET
#include "root_ca_cert.h"

#ifndef DEVICE_TRNG
#error "mbed-os-example-tls-socket requires a device which supports TRNG"
#endif
#endif // MBED_CONF_APP_USE_TLS_SOCKET

#define LD1_ON     {led1 = 1;}
#define LD1_OFF    {led1 = 0;}
#define LD2_ON     {led2 = 1;}
#define LD2_OFF    {led2 = 0;}

DigitalOut led1(LED1);
DigitalOut led2(LED2);

class SocketDemo {
    static constexpr size_t MAX_NUMBER_OF_ACCESS_POINTS = 10;
    static constexpr size_t MAX_MESSAGE_RECEIVED_LENGTH = 100;

#if MBED_CONF_APP_USE_TLS_SOCKET
    static constexpr size_t REMOTE_PORT = 65431; // tls port
#else
    static constexpr size_t REMOTE_PORT = 65431; // standard HTTP port
#endif // MBED_CONF_APP_USE_TLS_SOCKET

public:
    SocketDemo() : _net(NetworkInterface::get_default_instance())
    {
    }

    ~SocketDemo()
    {
        if (_net) {
            _net->disconnect();
        }
    }

    bool conn_wifi()
    {
        if (!_net) {
            printf("Error! No network interface found.\r\n");
            return false;
        }

        /* if we're using a wifi interface run a quick scan */
        if (_net->wifiInterface()) {
            /* the scan is not required to connect and only serves to show visible access points */
            wifi_scan();

            /* in this example we use credentials configured at compile time which are used by
             * NetworkInterface::connect() but it's possible to do this at runtime by using the
             * WiFiInterface::connect() which takes these parameters as arguments */
        }

        /* connect will perform the action appropriate to the interface type to connect to the network */

        printf("Connecting to the network...\r\n");
        
        nsapi_size_or_error_t result = _net->connect();
        if (result != 0) {
            printf("Error! _net->connect() returned: %d\r\n", result);
            return false;
        }

        print_network_info();

        /*opening the socket only allocates resources */
        
        result = _socket.open(_net);
        if (result != 0) {
            printf("Error! _socket.open() returned: %d\r\n", result);
            return false;
        }

#if MBED_CONF_APP_USE_TLS_SOCKET
        result = _socket.set_root_ca_cert(root_ca_cert);
        if (result != NSAPI_ERROR_OK) {
            printf("Error: _socket.set_root_ca_cert() returned %d\n", result);
            return false;
        }
        _socket.set_hostname("172.20.10.13");
#endif // MBED_CONF_APP_USE_TLS_SOCKET

        /* now we have to find where to connect */
        //address is a private data member
        if (!resolve_hostname(address)) {
            return false;
        }

        address.set_port(REMOTE_PORT);
        
        /* we are connected to the network but since we're using a connection oriented
         * protocol we still need to open a connection on the socket */
        
        printf("Opening connection to remote port %d\r\n", REMOTE_PORT);

        result = _socket.connect(address);
        if (result != 0) {
            printf("Error! _socket.connect() returned: %d\r\n", result);
            return false;
        }
        
        /*
        //collecting acc, then preprocess
        const int axis_num = 3;
        const int L = 300; //length of one gesture(data point)
        const int N = 9; //number of frame, (N+1) segments
        int Ls = int(L/(N+1));
        const int sample_interval = 5; //ms
        int16_t pDataXYZ[] = {0,0,0};
        float scale_multi = 0.01;
        float32_t R[axis_num][L];//raw acc data of one gesture
        char acc_json[100];
        int response;

        printf("start collecting...\n");
        int j = 0;
        while (j < L){
            BSP_ACCELERO_AccGetXYZ(pDataXYZ);
            for(int i = 0; i < axis_num; i++){
                R[i][j] = pDataXYZ[i];
            }
            j++;
            thread_sleep_for(sample_interval);
        }
        printf("end collecting.\n");
        
        main preprocess section
        features of one frame:
        mu,epsilon,delta,sigma, each has [x,y,z]
        gamma, has C(3,2) = 3, [x,y,z], for x is x with y
        features of all frames are stored by 3d array: feature
        
        float32_t feature[N][5][axis_num] = {0};
        arm_rfft_fast_instance_f32 fftins;
        arm_rfft_fast_init_f32(&fftins, 64);

        for (int fseq = 0; fseq < N; fseq++){
            for(int axis = 0; axis < axis_num; axis++){
                float32_t *timeframe = new float32_t[2*Ls];
                float32_t *natimeframe = new float32_t[2*Ls];
                float32_t *freqframe = new float32_t[2*Ls];
                float32_t freqframe_mag[Ls];
                for(int i = 0; i < 2*Ls; i++){
                    timeframe[i] = 0;
                    natimeframe[i] = 0;
                    freqframe[i] = 0;
                }
                for(int i = 0; i < 2*Ls; i++){
                    //extract frame data from R
                    timeframe[i] = R[axis][i+fseq*Ls];
                    natimeframe[i] = R[(axis+1)%3][i+fseq*Ls];         
                }
                arm_rfft_fast_f32(&fftins, timeframe, freqframe, 0);
                
                //mu,index 0--------------------------------------
                feature[fseq][0][axis] = freqframe[0];
                //epsilon,index 1---------------------------------
                float32_t magsum = 0;
                for(int i = 1; i < Ls; i++){
                    freqframe_mag[i] = sqrt(pow(freqframe[2*i],2)+pow(freqframe[2*i+1],2));
                    magsum += freqframe_mag[i];
                }
                for(int i = 1; i < Ls; i++){
                    feature[fseq][1][axis] += pow(freqframe_mag[i],2);
                }
                feature[fseq][1][axis] /= Ls-1;
                //delta,index 2-----------------------------------
                float32_t p;
                for(int i = 1; i < Ls; i++){
                    p = freqframe_mag[i]/magsum;
                    feature[fseq][2][axis] += p == 0 ? 0 : p*log(1/p);
                }
                //sigma,index 3-----------------------------------
                float32_t time_bar = 0;
                float32_t natime_bar = 0;
                float32_t nasigma = 0;
                for(int i = 0; i < 2*Ls; i++){
                    time_bar += timeframe[i];
                    natime_bar += natimeframe[i];
                }
                time_bar /= 2*Ls;
                natime_bar /= 2*Ls;
                for( int i = 0; i < 2*Ls; i++){
                    feature[fseq][3][axis] += pow(timeframe[i]-time_bar, 2);
                    nasigma += pow(natimeframe[i]-natime_bar, 2);
                }
                feature[fseq][3][axis] /= 2*Ls;
                nasigma /= 2*Ls;
                feature[fseq][3][axis] = sqrt(feature[fseq][3][axis]);
                nasigma = sqrt(nasigma);
                //gamma,index 4-----------------------------------
                for(int i = 0; i < 2*Ls; i++){
                    feature[fseq][4][axis] += (timeframe[i]-time_bar)*(natimeframe[i]-natime_bar);
                }
                feature[fseq][4][axis] /= 2*Ls*feature[fseq][3][axis]*nasigma;
                //delete
                delete [] timeframe;
                delete [] natimeframe;
                delete [] freqframe;
            }
        }
        */
        
        printf("Wifi connected.\r\n");
        return true;
    }

    bool send(int16_t** R, int sample_num)
    {
        //start sending
        char acc_json[100];
        int response;
        int len = sprintf(acc_json ,"%d", sample_num+1);
        _socket.send(acc_json,len);
        thread_sleep_for(30);
        //send sampledata to python server
        for(int i = 0; i <= sample_num; i ++){
            len = sprintf(acc_json ,"%d %d %d ", R[0][i], R[1][i], R[2][i]);
            response = _socket.send(acc_json,len);
            if (0 >= response){
                printf("Error seding: %d\n", response);
                return false;
            }
            thread_sleep_for(10);
        }
        
        return true;
        
    }

    // bool send(float32_t R[3][200], int sample_num)
    // {
    //     //start sending
    //     char acc_json[100];
    //     int response;
    //     int len = sprintf(acc_json ,"%d", sample_num+1);
    //     _socket.send(acc_json,len);
    //     thread_sleep_for(30);
    //     //send sampledata to python server
    //     for(int i = 0; i <= sample_num; i ++){
    //         len = sprintf(acc_json ,"%f %f %f ", R[0][i], R[1][i], R[2][i]);
    //         response = _socket.send(acc_json,len);
    //         if (0 >= response){
    //             printf("Error seding: %d\n", response);
    //             return false;
    //         }
    //         thread_sleep_for(10);
    //     }
        
    //     return true;
        
    // }


private:
    bool resolve_hostname(SocketAddress &address)
    {
        const char hostname[] = "172.20.10.13";

        /* get the host address */
        printf("\nResolve hostname %s\r\n", hostname);
        nsapi_size_or_error_t result = _net->gethostbyname(hostname, &address);
        if (result != 0) {
            printf("Error! gethostbyname(%s) returned: %d\r\n", hostname, result);
            return false;
        }

        printf("%s address is %s\r\n", hostname, (address.get_ip_address() ? address.get_ip_address() : "None") );

        return true;
    }

    bool send_http_request(const char* buffer)
    {
        /* loop until whole request sent */
        /*const char buffer[] = "GET / HTTP/1.1\r\n"
                              "Host: 192.168.43.219\r\n"
                              "Connection: close\r\n"
                              "\r\n";
        */
        nsapi_size_t bytes_to_send = strlen(buffer);
        nsapi_size_or_error_t bytes_sent = 0;

        printf("\r\nSending message: \r\n%s", buffer);

        while (bytes_to_send) {
            bytes_sent = _socket.send(buffer + bytes_sent, bytes_to_send);
            if (bytes_sent < 0) {
                printf("Error! _socket.send() returned: %d\r\n", bytes_sent);
                return false;
            } else {
                printf("sent %d bytes\r\n", bytes_sent);
            }

            bytes_to_send -= bytes_sent;
        }

        printf("Complete message sent\r\n");
        return true;
    }

    bool receive_http_response()
    {
        char buffer[MAX_MESSAGE_RECEIVED_LENGTH];
        int remaining_bytes = MAX_MESSAGE_RECEIVED_LENGTH;
        int received_bytes = 0;

        /* loop until there is nothing received or we've ran out of buffer space */
        nsapi_size_or_error_t result = remaining_bytes;
        while (result > 0 && remaining_bytes > 0) {
            result = _socket.recv(buffer + received_bytes, remaining_bytes);
            if (result < 0) {
                printf("Error! _socket.recv() returned: %d\r\n", result);
                return false;
            }
            received_bytes += result;
            remaining_bytes -= result;
        }

        /* the message is likely larger but we only want the HTTP response code */

        printf("received %d bytes:\r\n%.*s\r\n\r\n", received_bytes, strstr(buffer, "\n") - buffer, buffer);

        return true;
    }

    void wifi_scan()
    {
        WiFiInterface *wifi = _net->wifiInterface();

        WiFiAccessPoint ap[MAX_NUMBER_OF_ACCESS_POINTS];

        /* scan call returns number of access points found */
        int result = wifi->scan(ap, MAX_NUMBER_OF_ACCESS_POINTS);

        if (result <= 0) {
            printf("WiFiInterface::scan() failed with return value: %d\r\n", result);
            return;
        }

        printf("%d networks available:\r\n", result);

        for (int i = 0; i < result; i++) {
            printf("Network: %s secured: %s BSSID: %hhX:%hhX:%hhX:%hhx:%hhx:%hhx RSSI: %hhd Ch: %hhd\r\n",
                   ap[i].get_ssid(), get_security_string(ap[i].get_security()),
                   ap[i].get_bssid()[0], ap[i].get_bssid()[1], ap[i].get_bssid()[2],
                   ap[i].get_bssid()[3], ap[i].get_bssid()[4], ap[i].get_bssid()[5],
                   ap[i].get_rssi(), ap[i].get_channel());
        }
        printf("\r\n");
    }

    void print_network_info()
    {
        /* print the network info */
        SocketAddress a;
        _net->get_ip_address(&a);
        printf("IP address: %s\r\n", a.get_ip_address() ? a.get_ip_address() : "None");
        _net->get_netmask(&a);
        printf("Netmask: %s\r\n", a.get_ip_address() ? a.get_ip_address() : "None");
        _net->get_gateway(&a);
        printf("Gateway: %s\r\n", a.get_ip_address() ? a.get_ip_address() : "None");
    }

private:
    NetworkInterface *_net;
    SocketAddress address;

#if MBED_CONF_APP_USE_TLS_SOCKET
    TLSSocket _socket;
#else
    TCPSocket _socket;
#endif // MBED_CONF_APP_USE_TLS_SOCKET
};

#define TEST_LENGTH_SAMPLES  600

#define SNR_THRESHOLD_F32    75.0f
#define BLOCK_SIZE            32

#if defined(ARM_MATH_MVEF) && !defined(ARM_MATH_AUTOVECTORIZE)
/* Must be a multiple of 16 */
#define NUM_TAPS_ARRAY_SIZE              32
#else
#define NUM_TAPS_ARRAY_SIZE              29
#endif

#define NUM_TAPS              29

/* -------------------------------------------------------------------
 * Declare State buffer of size (numTaps + blockSize - 1)
 * ------------------------------------------------------------------- */
#if defined(ARM_MATH_MVEF) && !defined(ARM_MATH_AUTOVECTORIZE)
static float32_t firStateF32[2 * BLOCK_SIZE + NUM_TAPS - 1];
#else
static float32_t firStateF32[BLOCK_SIZE + NUM_TAPS - 1];
#endif 

#if defined(ARM_MATH_MVEF) && !defined(ARM_MATH_AUTOVECTORIZE)
const float32_t firCoeffs32[NUM_TAPS_ARRAY_SIZE] = {
  -0.0018225230f, -0.0015879294f, +0.0000000000f, +0.0036977508f, +0.0080754303f, +0.0085302217f, -0.0000000000f, -0.0173976984f,
  -0.0341458607f, -0.0333591565f, +0.0000000000f, +0.0676308395f, +0.1522061835f, +0.2229246956f, +0.2504960933f, +0.2229246956f,
  +0.1522061835f, +0.0676308395f, +0.0000000000f, -0.0333591565f, -0.0341458607f, -0.0173976984f, -0.0000000000f, +0.0085302217f,
  +0.0080754303f, +0.0036977508f, +0.0000000000f, -0.0015879294f, -0.0018225230f, 0.0f,0.0f,0.0f
};
#else
const float32_t firCoeffs32[NUM_TAPS_ARRAY_SIZE] = {
  -0.0018225230f, -0.0015879294f, +0.0000000000f, +0.0036977508f, +0.0080754303f, +0.0085302217f, -0.0000000000f, -0.0173976984f,
  -0.0341458607f, -0.0333591565f, +0.0000000000f, +0.0676308395f, +0.1522061835f, +0.2229246956f, +0.2504960933f, +0.2229246956f,
  +0.1522061835f, +0.0676308395f, +0.0000000000f, -0.0333591565f, -0.0341458607f, -0.0173976984f, -0.0000000000f, +0.0085302217f,
  +0.0080754303f, +0.0036977508f, +0.0000000000f, -0.0015879294f, -0.0018225230f
};
#endif

uint32_t blockSize = BLOCK_SIZE;
uint32_t numBlocks = TEST_LENGTH_SAMPLES/BLOCK_SIZE;

int main() {
    LD2_OFF;
    printf("\r\nGesture detection start.\r\n\r\n");
    BSP_ACCELERO_Init();

#ifdef MBED_CONF_MBED_TRACE_ENABLE
    mbed_trace_init();
#endif

    SocketDemo *WIFI = new SocketDemo();
    MBED_ASSERT(WIFI);
    while(!WIFI->conn_wifi()){
        delete WIFI;
        WIFI = new SocketDemo();
        thread_sleep_for(5000);
    }

    uint32_t i;
    arm_fir_instance_f32 S;
    arm_status status;
    float32_t  *inputF32, *outputF32;
    float32_t R_filtered[3][200];

    float32_t acc_data[200];

    arm_fir_init_f32(&S, NUM_TAPS, (float32_t *)&firCoeffs32[0], &firStateF32[0], blockSize);

    const int axis_num = 3;
    const int sample_interval = 5; //ms
    int16_t pDataXYZ[] = {0,0,0};
    int16_t **R = new int16_t*[axis_num]; //raw acc data of one gesture
    int16_t **R_pre = new int16_t*[axis_num]; //raw acc data of one gesture (pre_collect)
    int16_t **R_int = new int16_t*[axis_num]; //
    int Rcap = 300;
    int Rpcap = 50;
    int Rint = 5;
    for(int i = 0; i < axis_num; i++){
        R[i] = new int16_t[Rcap];
    }
    for(int i = 0; i < axis_num; i++){
        R_pre[i] = new int16_t[Rpcap];
    }
    //main repeat loop: listening -> get critical data -> send -> loop
    while(1){
        printf("observing acc...\n");
        for(int i = 0; i < Rpcap; i++){
            for(int j = 0; j < axis_num; j++){
                R_pre[j][i] = 0;
            }
        }
        while(1){
            BSP_ACCELERO_AccGetXYZ(pDataXYZ);

            for(int i = 0; i < Rpcap-1; i++){
                for(int j = 0; j < axis_num; j++){
                    R_pre[j][i] = R_pre[j][i+1];
                }
            }
            for(int j = 0; j < axis_num; j++){
                R_pre[j][Rpcap-1] = pDataXYZ[j];
            }
            int threshold = 1700; // unit: 0.01m/ss
            if(abs(pDataXYZ[0]) > threshold|| abs(pDataXYZ[1]) > threshold || abs(pDataXYZ[2]) > threshold){
                //printf("over threshold!\n");
                break;
            }
            thread_sleep_for(5);
        }    
        //thread_sleep_for(10);
        LD2_ON;
        //printf("start collecting...\n");
        int sample_num = 0;
        bool idle = false;
        const int tailnum = 20;
        int tailsum[axis_num] = {0};
        float taildiff[axis_num] = {0};
        while (!idle){
            BSP_ACCELERO_AccGetXYZ(pDataXYZ);
            //handle R overflow
            if(sample_num >= Rcap){
                int16_t *Rtemp = new int16_t[2*Rcap];
                for(int i = 0; i < axis_num; i++){
                    for(int j = 0; j < Rcap; j++){
                        Rtemp[j] = R[i][j];
                    }
                    delete [] R[i];
                    R[i] = Rtemp;
                }
                Rcap *= 2;
            }
            //detect idle
            for(int i = 0; i < axis_num; i++){
                R[i][sample_num] = pDataXYZ[i];
                if(sample_num >= tailnum){
                    tailsum[i] = 0;
                    taildiff[i] = 0;
                    for(int j = 0; j < tailnum; j++){
                        tailsum[i] += R[i][sample_num-j];
                    }
                    for(int j = 0; j < tailnum; j++){
                        taildiff[i] += abs(R[i][sample_num-j]-tailsum[i]/tailnum);
                    }
                }
            }
            if(sample_num >= 100){
                //printf("%f, %f, %f\n",taildiff[0]/tailnum,taildiff[1]/tailnum,taildiff[2]/tailnum);
                if(taildiff[0]/tailnum < 100 && taildiff[1]/tailnum < 100 && taildiff[2]/tailnum < 100){
                    idle = true;
                    printf("idle detected!\n");
                }
            }
            
            sample_num++;
            thread_sleep_for(sample_interval);
        }
        printf("end collecting.\n");
        LD2_OFF;



        int16_t **R_send = new int16_t*[axis_num];
        for (int i = 0; i < axis_num; i++){
            R_send[i] = new int16_t[sample_num+Rpcap];
        }
        for (int i = 0; i < axis_num; i++){
            for (int j = 0; j < sample_num+Rpcap; j++) {
                if(j < Rpcap){
                    R_send[i][j] = R_pre[i][j];
                }
                else{
                    R_send[i][j] = R[i][j-Rpcap];
                }
            }
        }
        for(int i = 0; i<axis_num; i++){
            for(int j = 0; j<200; j++){
                if(j >= sample_num+Rpcap-1){
                    acc_data[j] = R_send[i][sample_num+Rpcap-1];
                }else{
                    acc_data[j] = R_send[i][j];
                }
            }
            
            inputF32 = &acc_data[0];
            outputF32 = R_filtered[i];
            for(i=0; i < numBlocks; i++){
                arm_fir_f32(&S, inputF32 + (i * blockSize), outputF32 + (i * blockSize), blockSize);
            }
        }

        // WIFI->send(R_send, sample_num+Rpcap-40);

        // for(int i = 0; i < axis_num; i++){
        //     for(int j = 0; j < 200; j++){
        //        printf("%f\n", R_filtered[i][j]); 
        //     }
        // }

        if(!WIFI->send(R_send, sample_num+Rpcap-40)){
            delete WIFI;
            WIFI = new SocketDemo();
            WIFI->conn_wifi();
            //WIFI->send(R_pre, sample_num-80);
            WIFI->send(R_send, sample_num+Rpcap-40);
        }
        /*
        for(int i = 0; i < axis_num; i++){
            for(int j = 0; j < sample_num+Rpcap; j++){
               printf("%d\n", R_send[i][j]); 
            }
        }
        */
    }
    
    return 0;
}
