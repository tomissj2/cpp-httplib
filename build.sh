#!/bin/bash

# Define the image name and version
imageName="cpp-backend-image"
containerName="cppBackendContainer"
portMapping="8080:8080"

# Build the Docker image
docker build -t $imageName .

# Run the Docker container with the specified name and port mapping
docker run -d -p $portMapping --name $containerName $imageName

# Copy the /bin/client directory from the running container to the host
docker cp $containerName:/bin/client ./

# Remove the running container
docker rm -f $containerName

# Remove the Docker image
docker rmi -f $imageName