#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/printk.h>
#include <zephyr/logging/log.h>


//=======================================================
//LEDS EXTERNOS - Semaforo de pedestres
// Serve para separar o port d para uso, já que vamos implementar nele os LEDs externos
//=======================================================
static const struct device *port_d = DEVICE_DT_GET(DT_NODELABEL(gpiod)); // configura a porta a como uma variavel
#define led_vd 2    // pino D2 como pino do led verde externo
#define led_vm 0    // pino D0 como pino do led vermelho externo

//=======================================================
//LEDS DA PLACA - SEMAFORO DE CARROS
//=======================================================
#define LED_red DT_ALIAS(led2)
#define LED_green DT_ALIAS(led0)

//=======================================================
//configura os leds internos do sistema
//=======================================================
#if DT_NODE_HAS_STATUS(LED_green, okay)
static const struct gpio_dt_spec led_gr = GPIO_DT_SPEC_GET(LED_green, gpios);
#else
#error "Unsupported board: led0 devicetree alias is not defined"
#endif 


#if DT_NODE_HAS_STATUS(LED_red, okay)
static const struct gpio_dt_spec led_rd = GPIO_DT_SPEC_GET(LED_red, gpios);
#else
#error "Unsupported board: led2 devicetree alias is not defined"
#endif 

//=======================================================
// Configuração do botão
//=======================================================
#define BUTTON_NODE DT_NODELABEL(user_button_0)

#if DT_NODE_HAS_STATUS(BUTTON_NODE, okay)
static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET(BUTTON_NODE, gpios);
#else
#error "Unsupported board: user_button_0 devicetree alias is not defined"
#endif 
static struct gpio_callback button_cb_data;


volatile bool button_state=false; //variavel do estado do botão


// ISR - Toggle LED
void button_isr(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
    if(!button_state){
        //se o pedestre ainda não solicitou, marca a flag.
    printk("\n=== BOTÃO PRESSIONADO: PEDESTRE SOLICITOU TRAVESSIA ===\n");
    button_state=true;
    printk("\n estado do botao: %d\n", button_state);
    }
}



//=======================================================
//Modo de trabalho do código - alterar para ver diferentes comportamentos
//=======================================================
#define Modo_Normal 0
#define Modo_Travessia 1
#define Modo_Noturno 2
#define Modo_Atual Modo_Noturno





//semáforo pra controlar os estados do semaforo de pedestres
K_SEM_DEFINE(fila_ped1,0,1); //estado verde
K_SEM_DEFINE(fila_ped2,1,1); //estado vermelho




//semáforo pra controlar os estados do semaforo de carros
K_SEM_DEFINE(fila_car1,0,1); //estado vermelho
K_SEM_DEFINE(fila_car2,1,1); // estado verde
K_SEM_DEFINE(fila_car3,0,1); // estado amarelo



#if Modo_Atual == Modo_Travessia
    const char *modo = "Pedestre";

    void thread_vm_ex(void *p1, void *p2, void *p3) {
    while(1){
        
        k_sem_take(&fila_ped1, K_FOREVER);
        if(!button_state){
        
        gpio_pin_set(port_d, led_vm,1);   
        gpio_pin_set(port_d, led_vd,0);
        k_msleep(50); //dorme um pouco para liberar espaço para o funcionamento do outro led
        k_sem_give(&fila_ped1);
        }
        else{
            k_sem_give(&fila_ped2);
        }    
       
    } 
}


    void thread_vd_ex(void *p1, void *p2, void *p3) {
    while(1){
        
        
        if(button_state){
        k_sem_take(&fila_ped2, K_FOREVER);
        
        if(button_state){
        k_msleep(1000); // espera o amarelo concluir
        }
        gpio_pin_set(port_d, led_vd,1);
        gpio_pin_set(port_d, led_vm,0);
        button_state = false; 
        k_msleep(3000);
        // aqui a thread dorme para outras executarem.
        
        
        k_sem_give(&fila_ped1);
        }
    }     
}

    void thread_vd(void *p1, void *p2, void *p3) {

    while(1){
        
        k_sem_take(&fila_car2,K_FOREVER);
        if(!button_state){
          
        gpio_pin_set_dt(&led_rd,0);
        gpio_pin_set_dt(&led_gr,1);
            
        k_msleep(50);
        k_sem_give(&fila_car2);
        }
        else{
        
            k_sem_give(&fila_car3);
        }
       
    } 
}

    void thread_am(void *p1, void *p2, void *p3) {

    while(1){

        k_sem_take(&fila_car3,K_FOREVER);  
        
        gpio_pin_set_dt(&led_rd,1);
        gpio_pin_set_dt(&led_gr,1);
        printk("\nentrei no amarelo!\n");
        k_msleep(1000);

        k_sem_give(&fila_car1);
    } 
}

    void thread_vm(void *p1, void *p2, void *p3) {
        while(1){

        k_sem_take(&fila_car1,K_FOREVER);
        printk("\nentrei no semáforo vermelho!\n");
        gpio_pin_set_dt(&led_gr,0);
        gpio_pin_set_dt(&led_rd,1);
       
        
        k_msleep(3000);
        
        k_sem_give(&fila_car2);
    } 
}
K_THREAD_DEFINE(greenEx_tid, 512, thread_vd_ex, NULL, NULL, NULL, 5, 0, 0); //thread do led de pedestres verde
K_THREAD_DEFINE(redEx_tid, 512, thread_vm_ex, NULL, NULL, NULL, 5, 0, 0); //thread do led de pedestres vermelho


K_THREAD_DEFINE(green_tid, 512, thread_vd, NULL, NULL, NULL, 5, 0, 0); //thread do led de carros verde
K_THREAD_DEFINE(red_tid, 512, thread_vm, NULL, NULL, NULL, 5, 0, 0); //thread do led de carros amarelo
K_THREAD_DEFINE(yellow_tid, 512, thread_am, NULL, NULL, NULL, 5, 0, 0); //thread do led de carros vermelho

#elif Modo_Atual == Modo_Noturno
    const char *modo = "Noturno";

    void thread_vm_ex(void *p1, void *p2, void *p3) {
    while(1){
        k_sem_take(&fila_ped2, K_FOREVER);
       
        gpio_pin_toggle(port_d, led_vm);   
        k_sem_give(&fila_ped2);
        k_msleep(1000);

        //k_sem_give(&fila_ped2);
    } 
}

    void thread_am(void *p1, void *p2, void *p3) {
        while(1){

        k_sem_take(&fila_ped2, K_FOREVER);

        gpio_pin_toggle_dt(&led_rd);
        gpio_pin_toggle_dt(&led_gr);
        k_sem_give(&fila_ped2);   
        k_msleep(1000);

        //k_sem_give(&fila_ped2);
    } 
}
K_THREAD_DEFINE(redEx_tid, 512, thread_vm_ex, NULL, NULL, NULL, 5, 0, 0); //thread do led de pedestres vermelho
K_THREAD_DEFINE(red_tid, 512, thread_am, NULL, NULL, NULL, 5, 0, 0); //thread do led de carros amarelo



#elif Modo_Atual == Modo_Normal
    const char *modo = "Diurno";

    void thread_vd_ex(void *p1, void *p2, void *p3) {
    while(1){
        k_sem_take(&fila_ped1, K_FOREVER);

        gpio_pin_set(port_d, led_vd,1);
        gpio_pin_set(port_d, led_vm,0); 

        k_msleep(4000);

        k_sem_give(&fila_ped2); 
    }     
}

    void thread_vm_ex(void *p1, void *p2, void *p3) {
    while(1){
        k_sem_take(&fila_ped2, K_FOREVER);
       
        gpio_pin_set(port_d, led_vm,1);   
        gpio_pin_set(port_d, led_vd,0);

        k_msleep(4000);

        k_sem_give(&fila_ped1);
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
K_THREAD_DEFINE(greenEx_tid, 512, thread_vd_ex, NULL, NULL, NULL, 5, 0, 0); //thread do led de pedestres verde
K_THREAD_DEFINE(redEx_tid, 512, thread_vm_ex, NULL, NULL, NULL, 5, 0, 0); //thread do led de pedestres vermelho


K_THREAD_DEFINE(green_tid, 512, thread_vd, NULL, NULL, NULL, 5, 0, 0); //thread do led de carros verde
K_THREAD_DEFINE(red_tid, 512, thread_vm, NULL, NULL, NULL, 5, 0, 0); //thread do led de carros amarelo
K_THREAD_DEFINE(yellow_tid, 512, thread_am, NULL, NULL, NULL, 5, 0, 0); //thread do led de carros vermelho


#else
    const char *modo = "Erro: não foi possível inicializar um modo";

#endif

//void thread_btn(void *p1, void *p2, void *p3) {
//    while(1){
//        if(gpio_pin_get_dt(&button) != 0){ 
//            button_state++;
//            printk("click #%d\n", button_state);
//
//        }
//
//        k_msleep(300);
//    }   
//}


//K_THREAD_DEFINE(buttao_tid, 512, thread_btn, NULL, NULL, NULL, 3, 0, 0);


void main(void)
{ int ret_but, ret_led_vd, ret_led_vm, ret_led_ped_vm, ret_led_ped_vd;
    
    // Verificar se o botão está disponível

    if (!gpio_is_ready_dt(&button)) {
        printk("ERRO: Botão não está disponível!\n");
        return;
    }
    
    // Configurar botão como entrada COM pull-up forçado
    ret_but=gpio_pin_configure_dt(&button, GPIO_INPUT | GPIO_PULL_DOWN);
    if(ret_but<0){
        printk("ERRO: falha ao configurar botão!\n");
        return;
   }

   ret_led_vd = gpio_pin_configure_dt(&led_gr, GPIO_OUTPUT_INACTIVE);
   ret_led_vm = gpio_pin_configure_dt(&led_rd, GPIO_OUTPUT_INACTIVE);
    if(ret_led_vd < 0 || ret_led_vm < 0){
        printk("ERRO: Falha ao configurar GPIOs dos LEDs da placa\n");
        return;
   }        

    //gpio_pin_configure(port_a, led_am, GPIO_OUTPUT_INACTIVE); // configura o pino do led amarelo como saida
    ret_led_ped_vd = gpio_pin_configure(port_d, led_vd, GPIO_OUTPUT_INACTIVE); // configura o pino do led verde como saida
    ret_led_ped_vm = gpio_pin_configure(port_d, led_vm, GPIO_OUTPUT_INACTIVE); // configura o pino do led vermelho como saida

    if(ret_led_ped_vd < 0 || ret_led_ped_vm < 0){
        printk("ERRO: Falha ao configurar GPIOs dos LEDs externos\n");
        return;
   }     
    // Configurar interrupção na borda de descida
    gpio_pin_interrupt_configure_dt(&button, GPIO_INT_EDGE_FALLING);
    gpio_init_callback(&button_cb_data, button_isr, BIT(button.pin));
    gpio_add_callback(button.port, &button_cb_data);

    printk("\nInicializando placa no modo %c",*modo);
    k_sleep(K_FOREVER);
}