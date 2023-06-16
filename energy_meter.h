#define ADC0_IRQ_FIFO 22

struct sample_t{
    int16_t k;
    int16_t v[50];
    int16_t i[50];
};

int mean(int16_t *x, int16_t n){
    int32_t sum = 0;
    for(int i=0; i<n; i++){
        sum += x[i];
    }
    return sum/n;
}

int pot(int16_t *v, int16_t *i, int16_t v_med,  int16_t i_med, int16_t n){
    int32_t sum_pot = 0;
    for(int j=0; j<n; j++){
        sum_pot += (v[j] - v_med)*(i[j] - i_med);
    }
    return sum_pot/n;
}

float pot_reativa(float pot_ativa, float pot_aparente){
    return sqrt((pot_aparente*pot_aparente - pot_ativa*pot_ativa));
}

float rms(int16_t *x, int16_t n, int offset){
    int64_t sum = 0;
    for(int i=0; i<n; i++){
        sum += (x[i] - offset) * (x[i] - offset);
    }
    return sqrt(sum/n);
}

int k =0;

QueueHandle_t energy_meter_empty_queue;
QueueHandle_t energy_meter_full_queue;

void energy_meter_task(void *){
    while(true){

        sample_t *p = 0;
        if(!xQueueReceive(energy_meter_full_queue, &p, portMAX_DELAY)){
            continue;
        }
        
        absolute_time_t startTime = get_absolute_time();
        // for(int j=0; j<3000; j++){
        //     printf("%d, %d\n", V[j], I[j]);
        // }
        // k = 0;
        // printf("\n\n offset da tensão: %f\n\n", mean(V,3000));
        int mean_calc_v = mean(p->v,50);
        float rms_calc_v = rms(p->v, 50, mean_calc_v)*(1/5.17);

        int mean_calc_i = mean(p->i,50);
        float rms_calc_i = rms(p->i, 50, mean_calc_i)*(1/291.6);
        float pot_aparente = rms_calc_v*rms_calc_i;
        float pot_ativ = pot(p->v,p->i,mean_calc_v,mean_calc_i,50)*(1/5.17)*(1/291.6);
        float pot_reat = pot_reativa(pot_ativ,pot_aparente);

        printf("\033[J");
        // printf("Vmed | Imed ");
        printf("%d | %d | %f | %d | %f | %f | %f | %f \n",p->k,mean_calc_v,rms_calc_v,mean_calc_i,rms_calc_i,pot_aparente,pot_ativ,pot_reat);
        // printf("Média da tensão: %d\033[K\n\n", mean_calc_v);
        // printf("RMS da tensão: %f\033[K\n\n", rms_calc_v);
        // printf("Média da corrente: %d\033[K\n\n", mean_calc_i);
        // printf("RMS da corrente: %f\033[K\n\n", rms_calc_i);
        // printf("Potencia aparente: %f\033[K\n\n", pot_aparente);
        // printf("Potencia ativa: %f\033[K\n\n", pot_ativ);
        // printf("Potencia reativa: %f\033[K\n\n", pot_reat);
        // printf("V=");
        // for(int i=0; i<3000; i++){
        //     printf("%d,", V[i]);
        // }
        absolute_time_t endTime = get_absolute_time();

        printf("Tempo de execucao: %d\n", uint32_t(to_us_since_boot(endTime) - to_us_since_boot(startTime)));
        xQueueSend(energy_meter_empty_queue, &p, portMAX_DELAY);
    }
}

void energy_meter_init(){
    static sample_t BUFF[60];
    energy_meter_empty_queue = xQueueCreate(60, sizeof(sample_t *));
    energy_meter_full_queue = xQueueCreate(60, sizeof(sample_t *));
    for(int i=0; i<60; i++){
        sample_t *p = &BUFF[i];
        xQueueSend(energy_meter_empty_queue, &p, portMAX_DELAY);
    }
    xTaskCreate(&energy_meter_task, "task1", 512, 0, 1, NULL);
}

void energy_meter_isr(){
    static int k = 0;
    static int j = 0;
    static sample_t *p = 0;
    int v = adc_fifo_get(); //https://www.raspberrypi.com/documentation/pico-sdk/hardware.html#ga689d64744e7fe5284562569b24e9d624
    int i = adc_fifo_get();
    k++;
    irq_clear(ADC0_IRQ_FIFO);

    BaseType_t wake = 0;

    if(!p){
        if(!xQueueReceiveFromISR(energy_meter_empty_queue, &p, &wake)){
            return;
        }
        j = 0;
        p->k = k;
    }

    p->v[j] = v;
    p->i[j] = i;
    j++;

    if(j>=50){
        xQueueSendFromISR(energy_meter_full_queue, &p, &wake);
        p = 0;
    }
    
    portYIELD_FROM_ISR(wake);
    
}
   
