/*
 * speaker.c
 *
 * PC speaker/buzzer control.
 * 
 * Written & released by Keir Fraser <keir.xen@gmail.com>
 * 
 * This is free and unencumbered software released into the public domain.
 * See the file COPYING for more details, or visit <http://unlicense.org>.
 */

/* Timer 4, channel 3, PB8 */
#define gpio_spk gpiob
#define PIN_SPK 8
#define tim tim4
#define PWM_CCR ccr3

/* Period = 500us -> Frequency = 2000Hz */
#define ARR 499

/* Three volume settings, default is medium. PB5 selects others. */
static const unsigned int volumes[] = { 10, 30, 80 };
static unsigned int volume = 1;

static struct timer spk_timer;
static unsigned int nr_pulses;

static void spk_timer_fn(void *data)
{
    unsigned int ccr, next;

    if (nr_pulses & 1) {
        ccr = 0;
        next = stk_ms(120);
    } else {
        ccr = volumes[volume];
        next = stk_ms(30);
    }
    gpio_write_pin(gpioc, 13, !ccr); /* PC13 LED */
    tim->PWM_CCR = ccr;
    if (--nr_pulses)
        timer_set(&spk_timer, stk_deadline(next));
}

void speaker_init(void)
{
    /* Probe volume setting. */
    gpio_configure_pin(gpiob, 6, GPI_pull_up);
    gpio_configure_pin(gpiob, 5, GPO_pushpull(_2MHz, LOW));
    gpio_configure_pin(gpiob, 7, GPO_pushpull(_2MHz, HIGH));
    delay_ms(1);
    if (!gpio_read_pin(gpiob, 6)) {
        /* PB6 jumpered to PB5 -> low volume */
        volume = 0;
    } else {
        gpio_configure_pin(gpiob, 7, GPO_pushpull(_2MHz, LOW));
        delay_ms(1);
        if (!gpio_read_pin(gpiob, 6)) {
            /* PB6 jumpered to PB7 -> high volume */
            volume = 2;
        }
    }
   
    /* Set up timer, switch speaker off. PWM output is active high. */
    rcc->apb1enr |= RCC_APB1ENR_TIM4EN;
    tim->arr = ARR;
    tim->psc = SYSCLK_MHZ - 1; /* tick at 1MHz */
    tim->ccer = TIM_CCER_CC3E;
    tim->ccmr2 = (TIM_CCMR2_CC3S(TIM_CCS_OUTPUT) |
                  TIM_CCMR2_OC3M(TIM_OCM_PWM1)); /* PWM1: high then low */
    tim->PWM_CCR = tim->cr2 = tim->dier = 0;
    tim->cr1 = TIM_CR1_CEN;

    /* Set up the output pin. */
    afio->mapr |= AFIO_MAPR_TIM2_REMAP_PARTIAL_2;
    gpio_configure_pin(gpio_spk, PIN_SPK, AFO_pushpull(_2MHz));

    timer_init(&spk_timer, spk_timer_fn, NULL);
}

/* Volume: 0 (silence) - 20 (loudest) */
void speaker_pulses(uint8_t nr)
{
    /* Quadratic scaling of pulse width seems to give linear-ish volume. */
    tim->PWM_CCR = 250;
    nr_pulses = nr*2;
    timer_set(&spk_timer, stk_now());
}

/*
 * Local variables:
 * mode: C
 * c-file-style: "Linux"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
