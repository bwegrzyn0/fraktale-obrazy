CC = g++
OBJS= out/main.o out/imgui.o out/imgui_draw.o out/imgui_impl_sdl2.o out/imgui_impl_sdlrenderer2.o out/imgui_tables.o out/imgui_widgets.o
HEADERS = ./src/
TARGET = run.out
FLAGS = -w -ggdb3
LIBS = -lSDL2
run: $(OBJS)
	$(CC) -o $(TARGET) $(OBJS) $(LIBS) $(FLAGS)
out/main.o: src/main.cpp
	$(CC) -c $(FLAGS) $(LIBS) src/main.cpp -o out/main.o	
out/imgui.o: src/imgui/imgui.cpp
	$(CC) -c $(FLAGS) $(LIBS) src/imgui/imgui.cpp -o out/imgui.o	
out/imgui_draw.o: src/imgui/imgui_draw.cpp
	$(CC) -c $(FLAGS) $(LIBS) src/imgui/imgui_draw.cpp -o out/imgui_draw.o	
out/imgui_impl_sdl2.o: src/imgui/imgui_impl_sdl2.cpp
	$(CC) -c $(FLAGS) $(LIBS) src/imgui/imgui_impl_sdl2.cpp -o out/imgui_impl_sdl2.o	
out/imgui_impl_sdlrenderer2.o: src/imgui/imgui_impl_sdlrenderer2.cpp
	$(CC) -c $(FLAGS) $(LIBS) src/imgui/imgui_impl_sdlrenderer2.cpp -o out/imgui_impl_sdlrenderer2.o	
out/imgui_tables.o: src/imgui/imgui_tables.cpp
	$(CC) -c $(FLAGS) $(LIBS) src/imgui/imgui_tables.cpp -o out/imgui_tables.o	
out/imgui_widgets.o: src/imgui/imgui_widgets.cpp
	$(CC) -c $(FLAGS) $(LIBS) src/imgui/imgui_widgets.cpp -o out/imgui_widgets.o	


