powershell -Command "Remove-Item -Path '%1\build\assets' -Recurse -Force"
powershell -Command "Copy-Item -Path %1\assets\ -Destination %1\build\ -Recurse"