# ArduinoJson - https://arduinojson.org
# Copyright © 2014-2025, Benoit BLANCHON
# MIT License

add_executable(NumbersTests
	convertNumber.cpp
	decomposeFloat.cpp
	parseDouble.cpp
	parseFloat.cpp
	parseInteger.cpp
	parseNumber.cpp
)

add_test(Numbers NumbersTests)

set_tests_properties(Numbers
	PROPERTIES
		LABELS "Catch"
)
