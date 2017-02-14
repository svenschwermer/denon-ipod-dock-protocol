#include <numeric>
#include <iostream>
#include <vector>
#include <cstdint>
#include <mutex>
#include <chrono>
#include <memory>
#include <iomanip>
#include <thread>
#include <boost/asio.hpp>
#include "interpret.h"

using clk = std::chrono::high_resolution_clock;

inline unsigned codec_byte(uint8_t byte) { return byte ^ 0x31; }
inline std::vector<uint8_t> codec(const std::vector<uint8_t> & input)
{
	std::vector<uint8_t> result(input);
	std::for_each(result.begin(), result.end(), [](uint8_t &val){val ^= 0x31;});
	return result;
}

char printable_char(uint8_t c) { return (c > 0x20 && c < 0x7f) ? static_cast<char>(c) : ' '; }

void serialized_cout(const std::vector<uint8_t> & data, char id, const clk::time_point & t)
{
	static std::mutex mtx;
	static std::unique_ptr<clk::time_point> t_last = nullptr;

	std::unique_lock<std::mutex> lock(mtx);

	if(!t_last)
		t_last = std::make_unique<clk::time_point>(t);

	const auto delta_t = std::chrono::duration_cast<std::chrono::milliseconds>(t - *t_last);

	std::cout << std::dec << std::setfill(' ');
	std::cout << '[' << id << '|' << std::setw(9) << delta_t.count() << ']';
	std::cout << std::hex << std::setfill('0');

	auto interpreted = interpret(std::cout, data);

	auto acc = std::accumulate(data.begin() + 2, data.end(), static_cast<uint8_t>(0));
	if(acc != 0x00)
		std::cout << " [checksum error]";

	if(interpreted)
		std::cout << "\n             ";
	dump_raw(std::cout, data.begin(), data.end());

	std::cout << std::endl;
}

void read_serial(std::string dev_path, bool decode)
{
	using namespace boost;
	std::vector<uint8_t> buffer(3);

	asio::io_service io;
	asio::serial_port serial(io, dev_path);
	serial.set_option(asio::serial_port::baud_rate(19200));

	if(serial.is_open())
	{
		while(true)
		{
			asio::read(serial, asio::buffer(buffer, 3));
			const auto t = clk::now();

			unsigned len;
			if(decode)
				len = codec_byte(buffer[2]);
			else
				len = buffer[2];

			unsigned read_offset = 3;

			if(len == 0)
			{
				buffer.resize(read_offset + 2);
				asio::read(serial, asio::buffer(buffer.data() + read_offset, 2));

				if(decode)
					len = (static_cast<unsigned>(codec_byte(buffer[3])) << 8) | codec_byte(buffer[4]);
				else
					len = (static_cast<unsigned>(buffer[3]) << 8) | buffer[4];

				read_offset += 2;
			}

			buffer.resize(read_offset + len + 1);
			asio::read(serial, asio::buffer(buffer.data() + read_offset, buffer.size() - read_offset));

			if(decode)
			{
				std::vector<uint8_t> decoded_data = codec(buffer);
				serialized_cout(decoded_data, dev_path.back(), t);
			}
			else
				serialized_cout(buffer, dev_path.back(), t);
		}
	}
	else
		std::cerr << "Error opening \"" << dev_path << '"' << std::endl;
}

int main()
{
	std::thread t1(read_serial, "/dev/ttyUSB0", false);
	std::thread t2(read_serial, "/dev/ttyUSB1", false);
	std::thread t3(read_serial, "/dev/ttyUSB2", true);
	std::thread t4(read_serial, "/dev/ttyUSB3", true);

	t1.join();
	t2.join();
	t3.join();
	t4.join();

	return 0;
}
