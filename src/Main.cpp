
/* Created by LCBHSStudent */
/*       2020 Feb 3rd      */
#include "LCBHSS/lcbhss_space.h"
#include "VkApp/VkApp.h"
#include <iostream>
#include <exception>

int main(int argc, char* argv[]) {

	auto vkapp = new VkApp();
	try {
		vkapp->Run();
	}
	catch (const std::exception & err) {
		std::cerr << err.what() << std::endl;
		LOG_AND_RETURN(UNHANDLED_ERROR);
	}
	return 0;
}