#include <tamtypes.h>
#include <kernel.h>
#include <sifrpc.h>
#include <loadfile.h>
#include <stdio.h>
#include <libpad.h>

#define PAD_UP      0x0010
#define PAD_DOWN    0x0040
#define PAD_LEFT    0x0080
#define PAD_RIGHT   0x0020

#define ANALOG_THRESHOLD 40

// Buffer DMA para o pad (essencial para o PS2SDK)
static char padBuf[256] __attribute__((aligned(64)));

// =========================
// REMAP D-PAD (DIAGONAL)
// =========================
u16 remap_dpad(u16 pad) {
    u16 newpad = pad & ~(PAD_UP | PAD_DOWN | PAD_LEFT | PAD_RIGHT);

    // Lógica de prioridade para evitar conflitos
    if (pad & PAD_UP)         newpad |= (PAD_UP | PAD_LEFT);
    else if (pad & PAD_DOWN)  newpad |= (PAD_DOWN | PAD_RIGHT);
    else if (pad & PAD_LEFT)  newpad |= (PAD_LEFT | PAD_DOWN);
    else if (pad & PAD_RIGHT) newpad |= (PAD_RIGHT | PAD_UP);

    return newpad;
}

// =========================
// ANALÓGICO → DIGITAL
// =========================
u16 analog_to_dpad(struct padButtonStatus *buttons) {
    u16 out = 0;

    // Usando o analógico esquerdo (L3) conforme sua correção
    int lx = buttons->ljoy_h - 128;
    int ly = buttons->ljoy_v - 128;

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
    // 1. Aplica remap nas setas
    u16 dpad_remap = remap_dpad(raw);

    // 2. Pega analógico como digital
    u16 analog_dpad = analog_to_dpad(buttons);

    // 3. Junta tudo
    return dpad_remap | analog_dpad;
}

// =========================
// INICIALIZAÇÃO DE MÓDULOS
// =========================
void loadModules(void) {
    SifLoadModule("rom0:SIO2MAN", 0, NULL);
    SifLoadModule("rom0:PADMAN", 0, NULL);
}

// =========================
// MAIN
// =========================
int main() {
    int port = 0;
    int slot = 0;
    struct padButtonStatus buttons;

    // Inicializa RPC e módulos de controle
    SifInitRpc(0);
    loadModules();
    padInit(0);

    // Abre a porta do controle
    if (padPortOpen(port, slot, padBuf) == 0) {
        printf("Erro ao abrir a porta do controle!\n");
        return -1;
    }

    printf("App Remap PS2 Ativo!\n");
    printf("D-Pad Remapeado + Analógico Esquerdo Digital\n");

    while (1) {
        // Verifica se o controle está pronto
        int state = padGetState(port, slot);
        if (state == PAD_STATE_STABLE || state == PAD_STATE_FINDCTP1) {
            
            int ret = padRead(port, slot, &buttons);
            if (ret != 0) {
                // Inverte os bits (no PS2, 0 = pressionado)
                u16 raw = 0xFFFF ^ buttons.btns;
                u16 final = process_input(raw, &buttons);

                // Só imprime se houver alguma entrada
                if (raw != 0 || final != 0) {
                    printf("RAW: %04X | FINAL: %04X\n", raw, final);
                }
            }
        }

        // Pequeno delay para não sobrecarregar a CPU
        int i;
        for(i = 0; i < 10000; i++) { __asm__("nop"); }
    }

    return 0;
}
