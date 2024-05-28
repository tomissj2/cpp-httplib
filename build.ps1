# Define the image name and version
$imageName = "cpp-backend-image"
$containerName = "cppBackendContainer"
$portMapping = "8080:8080"

docker build -t $imageName .

docker run -d -p $portMapping --name $containerName $imageName

docker cp cppBackendContainer:/bin/client ./

docker rm -f $containerName

docker rmi -f $imageName