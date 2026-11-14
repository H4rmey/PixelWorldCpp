CXX = clang++
# Your compiler flags from earlier
CXXFLAGS = -std=c++23 -g -O0 -fno-omit-frame-pointer -DNOMINMAX -I.

# Linker flags for your libraries
LDFLAGS = -lenet -lsfml-graphics -lsfml-window -lsfml-system

# Output executable names
SERVER_BIN = server_app
CLIENT_BIN = client_app

# Shared sources (Code both the client and server need to compile)
SHARED_SRCS = world/chunk.cpp world/chunkManager.cpp
SHARED_OBJS = $(SHARED_SRCS:.cpp=.o)

# App-specific sources
SERVER_SRCS = cmd/server/main.cpp networking/server.cpp
CLIENT_SRCS = cmd/client/main.cpp networking/client.cpp

# Convert .cpp filenames to .o filenames
SERVER_OBJS = $(SERVER_SRCS:.cpp=.o)
CLIENT_OBJS = $(CLIENT_SRCS:.cpp=.o)

# 'make' with no arguments will build both by default
all: $(SERVER_BIN) $(CLIENT_BIN)

# Link the server executable
$(SERVER_BIN): $(SERVER_OBJS) $(SHARED_OBJS)
	$(CXX) $(SERVER_OBJS) $(SHARED_OBJS) -o $@ $(LDFLAGS)
	@echo "Built $(SERVER_BIN)"

# Link the client executable
$(CLIENT_BIN): $(CLIENT_OBJS) $(SHARED_OBJS)
	$(CXX) $(CLIENT_OBJS) $(SHARED_OBJS) -o $@ $(LDFLAGS)
	@echo "Built $(CLIENT_BIN)"

# Pattern rule: Compile any .cpp file into a distinct .o file
# NOTE: This specific rule fixes your `bear` issue by outputting distinct object files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# 'make clean' removes all compiled files
clean:
	rm -f $(SERVER_BIN) $(CLIENT_BIN) $(SERVER_OBJS) $(CLIENT_OBJS) $(SHARED_OBJS)

.PHONY: all clean
