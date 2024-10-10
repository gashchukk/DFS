# Use an official image with GCC pre-installed
FROM gcc:latest

# Set the working directory inside the container
WORKDIR /usr/src/app

# Copy the server source code to the container
COPY server.cpp .

# Compile the server program
RUN g++ -o server server.cpp

# Expose port 8080 for communication
EXPOSE 8080

# Run the server program
CMD ["./server"]
