# Use an official image with GCC pre-installed
FROM gcc:latest

# Set the working directory inside the container
WORKDIR /usr/src/app

# Copy the client source code to the container
COPY client.cpp .

# Compile the client program
RUN g++ -o client client.cpp

# Run the client program
CMD ["./client"]
