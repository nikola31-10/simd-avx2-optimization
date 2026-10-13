#include "descriptors.h"
#include "interrupts.h"
#include "io.h"
#include "shared_protocol.h"
#include "files.h"

static volatile int vm_role = -1;
static volatile int role_received = 0;
static struct idt_entry idt[IDT_ENTRIES];


static volatile int transfer_finished = 0;
static int file_fd = -1;

/*
	"general-regs-only" sprečava GCC da emituje SSE instrukcije, koje su zabranjene unutar
	__attribute__((interrupt)) handlera
*/
//static void __attribute__((interrupt, target("general-regs-only")))
//irq0_handler(struct interrupt_frame *frame)
static void __attribute__((interrupt))
irq0_handler(struct interrupt_frame *frame)
{
	const char *s;

	(void)frame;

	/*
	 * Prvi prekid služi isključivo za dobijanje uloge.
	 */
	if (!role_received) {
		vm_role = (int)inl(SHARED_IO_PORT);
		role_received = 1;

		if (vm_role == ROLE_WRITER) {
			for (s = "Role: WRITER\n"; *s; ++s)
				outb(0xE9, *s);

			file_fd = open("input.txt", FILE_O_RD);
		} else if (vm_role == ROLE_READER) {
			for (s = "Role: READER\n"; *s; ++s)
				outb(0xE9, *s);

			file_fd = open(
				"output.txt",
				FILE_O_WR | FILE_O_CREATE
			);
		}

		if (file_fd < 0) {
			for (s = "File open failed\n"; *s; ++s)
				outb(0xE9, *s);

			transfer_finished = 1;
		}

		return;
	}

	/*
	 * Svaki sledeći IRQ obavlja jednu rundu prenosa.
	 */
	if (transfer_finished)
		return;

	if (vm_role == ROLE_WRITER) {
		char buffer[BUFFER_SIZE];
		int count;
		uint32_t accepted;
		int i;

		count = read(file_fd, buffer, BUFFER_SIZE);

		if (count < 0) {
			for (s = "Writer file read failed\n"; *s; ++s)
				outb(0xE9, *s);

			transfer_finished = 1;
			close(file_fd);
			file_fd = -1;
			return;
		}

		/* Šalje broj bajtova. */
		outl(SHARED_IO_PORT, (uint32_t)count);

		/* Šalje sadržaj. */
		for (i = 0; i < count; i++)
			outb(SHARED_IO_PORT, (uint8_t)buffer[i]);

		/* Čeka da svi readeri završe. */
		do {
			accepted = inl(SHARED_ACK_PORT);
		} while (accepted == SHARED_NOT_READY);

		if (accepted != (uint32_t)count) {
			for (s = "Writer ACK failed\n"; *s; ++s)
				outb(0xE9, *s);

			transfer_finished = 1;
			close(file_fd);
			file_fd = -1;
			return;
		}

		/*
		* Ako je pročitano manje od BUFFER_SIZE,
		* ovo je poslednji blok fajla.
		*/
		if (count < BUFFER_SIZE) {
			for (s = "Writer transfer finished\n"; *s; ++s)
				outb(0xE9, *s);

			transfer_finished = 1;
			close(file_fd);
			file_fd = -1;
		}
	}
	else if (vm_role == ROLE_READER) {
		char buffer[BUFFER_SIZE];
		uint32_t count;
		uint32_t i;
		int written;

		/* Čeka da writer pripremi blok. */
		do {
			count = inl(SHARED_IO_PORT);
		} while (count == SHARED_NOT_READY);

		if (count > BUFFER_SIZE) {
			for (s = "Reader invalid block size\n"; *s; ++s)
				outb(0xE9, *s);

			transfer_finished = 1;
			close(file_fd);
			file_fd = -1;
			return;
		}

		for (i = 0; i < count; i++)
			buffer[i] = (char)inb(SHARED_IO_PORT);

		written = write(file_fd, buffer, (int)count);

		if (written != (int)count) {
			for (s = "Reader file write failed\n"; *s; ++s)
				outb(0xE9, *s);

			transfer_finished = 1;
			close(file_fd);
			file_fd = -1;
			return;
		}

		/* Obaveštava host koliko bajtova je obradio. */
		outl(SHARED_ACK_PORT, count);

		if (count < BUFFER_SIZE) {
			for (s = "Reader transfer finished\n"; *s; ++s)
				outb(0xE9, *s);

			transfer_finished = 1;
			close(file_fd);
			file_fd = -1;
		}
	}
}

static void set_idt_gate(unsigned n, void (*handler)(struct interrupt_frame *))
{
	uint64_t addr = (uint64_t)(uintptr_t)handler;
	idt[n].offset_low  = addr & 0xFFFF;
	idt[n].selector    = 0x08;  /* 64-bit code segment */
	idt[n].ist         = 0;
	idt[n].type_attr   = 0x8E;  /* P=1, DPL=0, 64-bit interrupt gate */
	idt[n].offset_mid  = (addr >> 16) & 0xFFFF;
	idt[n].offset_high = (addr >> 32) & 0xFFFFFFFF;
	idt[n].reserved    = 0;
}

void init_idt(void)
{
	struct dt_ptr p;

	set_idt_gate(32, irq0_handler);

	p.limit = sizeof(idt) - 1;
	p.base  = (uint64_t)(uintptr_t)idt;
	asm volatile("lidt %0" : : "m"(p) : "memory");
}
