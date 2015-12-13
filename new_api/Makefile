TARGET=sample
all:
	clang++ $(TARGET).cpp -o $(TARGET) -g `llvm-config --cxxflags --ldflags --system-libs --libs all`
run:
	@./$(TARGET)
clean:
	rm -f $(TARGET)
