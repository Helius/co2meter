add_avr_library(
  mhz19
	mhz19.h
	mhz19.cpp
)

avr_target_link_libraries(mhz19 hallib)
