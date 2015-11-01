#include <fstream>
#include <sstream>
#include <iomanip>

#include "chip8-cpu.h"
#include "chip8-memory.h"
#include "sfTextTools.h"


void chip8::initCpu()
{
	std::fill_n(stack, 16, 0);
	opcode ,
		I = 0;
	pc = 0x200; // Starting adress where the game is loaded
	sp = 0;
	delay_timer = 0;
	sound_timer = 0;
}

void chip8::initialize()
{
	initCpu();
	initMem();
}

int chip8::loadGame(const char* name) const
{
	std::ifstream game(name, std::ios::in | std::ios::binary | std::ios::ate);
	std::streamsize size = game.tellg();
	game.seekg(0, std::ios::beg);

	//Fill the memory with game data at location: 0x200 == 512
	game.read(reinterpret_cast<char*>(mem::memory) + 0x200, size);

	return int(game.gcount());
}

// If this returns false, we need to stop the emulation
bool chip8::emulateCycle()
{
	//Fetch opcode
	opcode = mem::memory[pc] << 8 |
		mem::memory[pc + 1];

	//Decode opcode
	//If decodeOpcode returns false, return false
	if (decodeOpcode(opcode))
	{
	}
	else return false;

	// Update timers
	if (delay_timer > 0)
		--delay_timer;

	if (sound_timer > 0)
	{
		if (sound_timer == 1)
			appendText(&debugText, "BEEP!");
		--sound_timer;
	}
	return true;
}

bool chip8::decodeOpcode(unsigned short opcode)
{
	using namespace mem;	// We're gonna be using registers (mem::V[]) a lot

	switch (opcode & 0xF000)
	{
	case 0x0000:
		switch (opcode & 0x0FFF)
		{
		case 0x00E0: // Clear screen

			pc += 2;
			break;
		case 0x00EE: // Return from a subroutine
			pc = stack[sp--] + 2;
			break;
		}
	case 0x1000: // (1NNN) Jumps to address NNN
		pc = opcode & 0x0FFF;
		break;
	case 0x2000: // (2NNN) Calls subroutine at NNN
		stack[sp++] = pc;
		pc = opcode & 0x0FFF;
		break;
	case 0x3000: // (3XNN) Skips the next instruction if VX equals NN
		if (V[(opcode & 0x0F00)] == (opcode & 0x00FF))
		{
			pc += 4;
		}
		break;
	case 0x4000: // (4XNN) Skips the next instruction if VX doesn't equal NN
		if (V[(opcode & 0x0F00)] != (opcode & 0x00FF))
		{
			pc += 4;
		}
		break;
	case 0x5000: // (5XN0) Skips the next instruction if VX equals VY
		switch (opcode & 0x000F)
		{
		case 0x0000:
			if (V[(opcode & 0x0F00)] == V[(opcode & 0x00F0)])
			{
				pc += 4;
			}
		}
	case 0x6000: // (6XNN) Sets VX to NN
		V[(opcode & 0x0F00)] = (opcode & 0x00FF);
		pc += 2;
		break;
	case 0x7000: // (7XNN) Adds NN to VX.
		V[(opcode & 0x0F00)] += (opcode & 0x00FF);
		pc += 2;
		break;
	case 0x8000: // (8XY0) Sets VX to the value of VY.
		switch (opcode & 0x000F)
		{
		case 0x0000:
			V[(opcode & 0x0F00)] = V[(opcode & 0x00F0)];
			pc += 2;
			break;
		case 0x0001: // (8XY1) Sets VX to VX or VY.
			V[(opcode & 0x0F00)] = V[(opcode & 0x0F00) |
				(opcode & 0x00F0)];
			pc += 2;
			break;
		case 0x0002: // (8XY2) Sets VX to VX and VY.
			V[(opcode & 0x0F00)] = V[(opcode & 0x0F00) &
				(opcode & 0x00F0)];
			pc += 2;
			break;
		case 0x0003: // (8XY3) Sets VX to VX xor VY.
			V[(opcode & 0x0F00)] = V[(opcode & 0x0F00) ^
				(opcode & 0x00F0)];
			pc += 2;
			break;
		case 0x0004: // (8XY4) Adds VY to VX. VF is set to 1 when there's a carry,
					 // and to 0 when there isn't.
			if (V[(opcode & 0x00F0) >> 4] > (0xFF - V[(opcode & 0x0F00) >> 8]))
				V[0xF] = 1; //carry
			else
				V[0xF] = 0;
			V[(opcode & 0x0F00) >> 8] += V[(opcode & 0x00F0) >> 4];
			pc += 2;
			break;
		case 0x0005: // (8XY5) VY is subtracted from VX. VF is set to 0 when there's a borrow,
					 // and to 1 when there isn't
			if (V[(opcode & 0x00F0) >> 4] > (V[(opcode & 0x0F00) >> 8]))
				V[0xF] = 0; //borrow
			else
				V[0xF] = 1;
			V[(opcode & 0x0F00) >> 8] -= V[(opcode & 0x00F0) >> 4];
			pc += 2;
			break;
		case 0x0006: // (8XY6) Shifts VX right by one. 
					 // VF is set to the value of the least significant bit of VX before the shift
			V[0xF] = V[opcode & 0x0F00] & 1;
			V[opcode & 0x0F00] >>= 1;

		case 0x0007: // (8XY7) Sets VX to VY minus VX. VF is set to 0 when there's a borrow,
					 // and 1 when there isn't
			if (V[(opcode & 0x00F0) >> 4] < (V[(opcode & 0x0F00) >> 8]))
				V[0xF] = 0; //borrow
			else
				V[0xF] = 1;
			V[(opcode & 0x0F00) >> 8] = V[(opcode & 0x00F0) >> 4] - V[(opcode & 0x0F00) >> 8];
			pc += 2;
			break;
		case 0x000E: // (8XYE) Shifts VX left by one. 
					 // VF is set to the value of the most significant bit of VX before the shift
			V[0xF] = V[opcode & 0x0F00] & 128; // 128 comes from this:
											   // (1 << (sizeof(unsigned char) * 8 - 1));
			V[opcode & 0x0F00] <<= 1;
			pc += 2;
			break;
		}
	case 0x9000: // (9XY0) Skips the next instruction if VX doesn't equal VY.
		if (V[(opcode & 0x0F00) >> 8] != V[(opcode & 0x00F0) >> 4])
		{
			pc += 2;
		}
		pc += 2;
		break;
	case 0xA000: // (ANNN) Sets I to the address NNN
		I = opcode & 0x0FFF;
		pc += 2;
		break;
	case 0xB000: // (BNNN) Jumps to the address NNN plus V0.
		pc = (opcode & 0x0FFF) + V[0x0];
		break;
	case 0xC000: // (CXNN) Sets VX to the result of a bitwise and operation
				 // on a random number and NN.
		V[(opcode & 0x0F00) >> 12] = (rand() & 0x00FF) & (opcode & 0x00FF);
	case 0xD000: //TODO
	case 0xE000:
		switch (opcode & 0x00FF)
		{
		case 0x009E: // (EX9E) Skips the next instruction if the key stored in VX is pressed.
		case 0x00A1: // (EX9E) Skips the next instruction if the key stored in VX isn't pressed.
		{}
		}
	case 0xF000: //TODO
		switch (opcode & 0x00FF)
		{
		case 0x0007: // (FX07) Sets VX to the value of the delay timer.
			V[(opcode & 0x0F00) >> 12] = delay_timer;
		case 0x000A: // TODO: (FX0A) A key press is awaited, and then stored in VX.
		case 0x0015: // (FX15) Sets the delay timer to VX.
			delay_timer = V[(opcode & 0x0F00) >> 12];
		case 0x0018: // (FX18) Sets the sound timer to VX.
			sound_timer = V[(opcode & 0x0F00) >> 12];
		case 0x001E: // (FX1E) Adds VX to I. VF is set to 1 when there's a carry,
					 // and to 0 when there isn't
			if (V[(opcode & 0x0F00) >> 8] > (0xFF - I))
				V[0xF] = 1; //carry
			else
				V[0xF] = 0;
			I += V[(opcode & 0x0F00) >> 8];
			pc += 2;
			break;

		case 0x0029:
		case 0x0033:
		case 0x0055:
		case 0x0065:
		{}
		}


	default:
		std::ostringstream opcode_ss;
		opcode_ss << std::hex << std::setfill('0') << std::uppercase <<
			"Unknown opcode: 0x" << std::setw(4) << opcode;

		appendText(&debugText, &opcode_ss);
		return false; // We can't handle this opcode, so stop the emulation
	}
	return true;
}

void chip8::stopEmulation()
{
	isRunning = false;
	appendText(&debugText, "Emulation stopped");
}
