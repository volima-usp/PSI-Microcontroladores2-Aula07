#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>

// Configuração do botão
#define BUTTON_NODE DT_NODELABEL(user_button_0)
static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET(BUTTON_NODE, gpios);

#define led_vd 2    // pino D2 como pino do led verde externo
#define led_vm 0    // pino D0 como pino do led vermelho externo

#define LED_red DT_ALIAS(led2)
#define LED_green DT_ALIAS(led0)

#define Modo_Normal 0
#define Modo_Travessia 1
#define Modo_Noturno 2
#define Modo_Atual Modo_Travessia

static const struct device *port_d = DEVICE_DT_GET(DT_NODELABEL(gpiod)); // configura a porta a como uma variavel

static const struct gpio_dt_spec led_gr = GPIO_DT_SPEC_GET(LED_green, gpios);
static const struct gpio_dt_spec led_rd = GPIO_DT_SPEC_GET(LED_red, gpios);

volatile int button_state = 0;

K_SEM_DEFINE(fila_ped,1,1);

K_SEM_DEFINE(fila_car1,0,1);
K_SEM_DEFINE(fila_car2,1,1);
K_SEM_DEFINE(fila_car3,0,1);

#if Modo_Atual == Modo_Travessia
    const char *modo = "Pedestre";

    void thread_vm_ex(void *p1, void *p2, void *p3) {
    while(1){
        k_sem_take(&fila_ped, K_FOREVER);
       
        gpio_pin_set(port_d, led_vm,1);   
        gpio_pin_set(port_d, led_vd,0);

        k_msleep(1000);

        k_sem_give(&fila_ped);
    } 
}

    void thread_vd_ex(void *p1, void *p2, void *p3) {
    while(1){
        if(button_state != 0){
        k_sem_take(&fila_ped, K_FOREVER);

        gpio_pin_set(port_d, led_vd,1);
        gpio_pin_set(port_d, led_vm,0); 

        k_msleep(4000);
        }
    }     
}

    void thread_vd(void *p1, void *p2, void *p3) {

    while(1){

        k_sem_take(&fila_car2, K_FOREVER);  
           
        gpio_pin_set_dt(&led_gr,1);
            
        k_msleep(500);

    } 
}

    void thread_am(void *p1, void *p2, void *p3) {

    while(1){

        k_sem_take(&fila_car3, K_FOREVER);  
           
        gpio_pin_set_dt(&led_rd,1);
        gpio_pin_set_dt(&led_gr,1);

        k_msleep(500);

        k_sem_give(&fila_car1);
    } 
}

    void thread_vm(void *p1, void *p2, void *p3) {
        while(1){

        k_sem_take(&fila_car1, K_FOREVER);

        gpio_pin_set_dt(&led_rd,1);
        gpio_pin_set_dt(&led_gr,0);

        k_sleep(K_FOREVER);

        k_sem_give(&fila_car2);
    } 
}


#elif Modo_Atual == Modo_Noturno
    const char *modo = "Noturno";

    void thread_vm_ex(void *p1, void *p2, void *p3) {
    while(1){
        k_sem_take(&fila_ped, K_FOREVER);
       
        gpio_pin_toggle(port_d, led_vm);   

        k_msleep(500);

        k_sem_give(&fila_ped);
    } 
}

    void thread_am(void *p1, void *p2, void *p3) {
        while(1){

        k_sem_take(&fila_ped, K_FOREVER);

        gpio_pin_toggle_dt(&led_rd);
        gpio_pin_toggle_dt(&led_gr);

        k_msleep(500);

        k_sem_give(&fila_ped);
    } 
}


#elif Modo_Atual == Modo_Normal
    const char *modo = "Diurno";

    void thread_vd_ex(void *p1, void *p2, void *p3) {
    while(1){
        k_sem_take(&fila_ped, K_FOREVER);

        gpio_pin_set(port_d, led_vd,1);
        gpio_pin_set(port_d, led_vm,0); 

        k_msleep(4000);

        k_sem_give(&fila_ped); 
    }     
}

    void thread_vm_ex(void *p1, void *p2, void *p3) {
    while(1){
        k_sem_take(&fila_ped, K_FOREVER);
       
        gpio_pin_set(port_d, led_vm,1);   
        gpio_pin_set(port_d, led_vd,0);

        k_msleep(4000);

        k_sem_give(&fila_ped);
    } 
}

    void thread_vm(void *p1, void *p2, void *p3) {
        while(1){

        k_sem_take(&fila_car1, K_FOREVER);

        gpio_pin_set_dt(&led_rd,1);
        gpio_pin_set_dt(&led_gr,0);

        k_msleep(4000);

        k_sem_give(&fila_car2);
    } 
}

    void thread_vd(void *p1, void *p2, void *p3) {

    while(1){

        k_sem_take(&fila_car2, K_FOREVER);  
           
        gpio_pin_set_dt(&led_rd,0);
        gpio_pin_set_dt(&led_gr,1);
            
        k_msleep(3000);

        k_sem_give(&fila_car3);
    } 
}

    void thread_am(void *p1, void *p2, void *p3) {

    while(1){

        k_sem_take(&fila_car3, K_FOREVER);  
           
        gpio_pin_set_dt(&led_rd,1);
        gpio_pin_set_dt(&led_gr,1);

        k_msleep(1000);

        k_sem_give(&fila_car1);
    } 
}


#else
    const char *modo = "Erro";

#endif

void thread_btn(void *p1, void *p2, void *p3) {
    while(1){
        if(gpio_pin_get_dt(&button) != 0){ 
            button_state++;
            printk("click #%d\n", button_state);
            if(button_state != 0 ) k_sem_give(&fila_car3);

        }

        k_msleep(300);
    }   
}

K_THREAD_DEFINE(greenEx_tid, 512, thread_vd_ex, NULL, NULL, NULL, 5, 0, 0);
K_THREAD_DEFINE(redEx_tid, 512, thread_vm_ex, NULL, NULL, NULL, 5, 0, 0);

K_THREAD_DEFINE(green_tid, 512, thread_vd, NULL, NULL, NULL, 5, 0, 0);
K_THREAD_DEFINE(red_tid, 512, thread_vm, NULL, NULL, NULL, 5, 0, 0);
K_THREAD_DEFINE(yellow_tid, 512, thread_am, NULL, NULL, NULL, 5, 0, 0);


K_THREAD_DEFINE(buttao_tid, 512, thread_btn, NULL, NULL, NULL, 3, 0, 0);


void main(void)
{
    
    // Verificar se o botão está disponível
    if (!gpio_is_ready_dt(&button)) {
        printk("ERRO: Botão não está disponível!\n");
        return;
    }
    
    // Configurar botão como entrada COM pull-up forçado
    gpio_pin_configure_dt(&button, GPIO_INPUT | GPIO_PULL_UP);

    gpio_pin_configure_dt(&led_gr, GPIO_OUTPUT_INACTIVE);
    gpio_pin_configure_dt(&led_rd, GPIO_OUTPUT_INACTIVE);
            

    //gpio_pin_configure(port_a, led_am, GPIO_OUTPUT_INACTIVE); // configura o pino do led amarelo como saida
    gpio_pin_configure(port_d, led_vd, GPIO_OUTPUT_INACTIVE); // configura o pino do led verde como saida
    gpio_pin_configure(port_d, led_vm, GPIO_OUTPUT_INACTIVE); // configura o pino do led vermelho como saida

    k_sleep(K_FOREVER);
}