#include <tamtypes.h>
#include <kernel.h>
#include <sifrpc.h>
#include <loadfile.h>
#include <stdio.h>
#include <libpad.h>

#define PAD_UP    0x0010
#define PAD_DOWN  0x0040
#define PAD_LEFT  0x0080
#define PAD_RIGHT 0x0020

#define ANALOG_THRESHOLD 40

// Buffer DMA para o pad
static char padBuf[256] __attribute__((aligned(64)));

// =========================
// REMAP D-PAD (DIAGONAL)
// =========================
u16 remap_dpad(u16 pad) {

    u16 newpad = pad & ~(PAD_UP | PAD_DOWN | PAD_LEFT | PAD_RIGHT);

    if (pad & PAD_DOWN)  newpad |= (PAD_DOWN | PAD_RIGHT);
    if (pad & PAD_UP)    newpad |= (PAD_UP | PAD_LEFT);
    if (pad & PAD_LEFT)  newpad |= (PAD_LEFT | PAD_DOWN);
    if (pad & PAD_RIGHT) newpad |= (PAD_RIGHT | PAD_UP);

    return newpad;
}

// =========================
// ANALÓGICO → DIGITAL
// =========================
u16 analog_to_dpad(struct padButtonStatus *buttons) {

    u16 out = 0;

    int lx = buttons->rjoy_h - 128;
    int ly = buttons->rjoy_v - 128;

    if (ly < -ANALOG_THRESHOLD) out |= PAD_UP;
    if (ly >  ANALOG_THRESHOLD) out |= PAD_DOWN;
    if (lx < -ANALOG_THRESHOLD) out |= PAD_LEFT;
    if (lx >  ANALOG_THRESHOLD) out |= PAD_RIGHT;

    return out;
}

// =========================
// PROCESSAMENTO FINAL
// =========================
u16 process_input(u16 raw, struct padButtonStatus *buttons) {

    // 1. aplica remap nas setas
    u16 dpad_remap = remap_dpad(raw);

    // 2. pega analógico como digital
    u16 analog_dpad = analog_to_dpad(buttons);

    // 3. junta tudo
    return dpad_remap | analog_dpad;
}

// =========================
// CARREGAR MÓDULOS
// =========================
void loadModules(void) {
    int ret;

    ret = SifLoadModule("rom0:SIO2MAN", 0, NULL);
    if (ret < 0) {
        printf("sifLoadModule sio failed: %d\n", ret);
        SleepThread();
    }

    ret = SifLoadModule("rom0:PADMAN", 0, NULL);
    if (ret < 0) {
        printf("sifLoadModule pad failed: %d\n", ret);
        SleepThread();
    }
}

// =========================
// MAIN
// =========================
int main() {
    int ret;
    int port = 0; // Porta 1
    int slot = 0; // Slot 1
    struct padButtonStatus buttons;

    // Inicializa RPC
    SifInitRpc(0);

    // Carrega módulos necessários para o controle
    loadModules();

    // Inicializa a biblioteca do pad
    padInit(0);

    // Abre a porta do pad
    if((ret = padPortOpen(port, slot, padBuf)) == 0) {
        printf("padOpenPort failed: %d\n", ret);
        SleepThread();
    }

    printf("Remap + Analog Digital ativo!\n");

    while (1) {
        // Verifica o estado do pad
        int state = padGetState(port, slot);
        if (state == PAD_STATE_STABLE || state == PAD_STATE_FINDCTP1) {
            
            ret = padRead(port, slot, &buttons);

            if (ret != 0) {
                // Os botões no PS2 são ativos em 0, então invertemos para facilitar
                u16 raw = 0xFFFF ^ buttons.btns;

                u16 final = process_input(raw, &buttons);

                // Apenas imprime se algum botão estiver pressionado
                if (raw != 0 || final != 0) {
                    printf("RAW: %04X | FINAL: %04X\n", raw, final);
                }
            }
        }
        
        // Pequeno delay para não sobrecarregar a CPU
        // Em um jogo real, isso estaria sincronizado com o VSync
        int i;
        for(i=0; i<10000; i++) { asm("nop"); }
    }

    return 0;
}
