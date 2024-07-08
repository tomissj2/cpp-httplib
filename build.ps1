# Define the image name and version
$imageName = "cpp-backend-image"
$containerName = "cppBackendContainer"
$portMapping = "8080:8080"

docker build -t $imageName .

docker run -d -p $portMapping --name $containerName $imageName

docker cp cppBackendContainer:/bin/client ./../jor_v1/cpp-app/build/bin/
docker cp cppBackendContainer:/bin/client ./
docker cp cppBackendContainer:/bin/server ./../jor_v1/cpp-app/build/bin/
docker cp cppBackendContainer:/bin/server ./

# docker cp cppBackendContainer:/bin/client jor-routing-server-cpp:/home/bin/
# docker cp cppBackendContainer:/bin/server jor-routing-server-cpp:/home/bin/

docker rm -f $containerName

docker rmi -f $imageNamepo