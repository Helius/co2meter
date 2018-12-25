add_avr_library(
  ssd1306
  SSD1306.cpp
  SSD1306.h
)

avr_target_link_libraries(ssd1306 hallib)
