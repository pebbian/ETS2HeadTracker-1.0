#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <string>
#include <thread>

#pragma comment(lib, "ws2_32.lib")

#pragma pack(push, 1)
struct FTData {
    int DataID;
    int CamWidth;
    int CamHeight;
    float Yaw;
    float Pitch;
    float Roll;
    float X;
    float Y;
    float Z;
    float RawYaw;
    float RawPitch;
    float RawRoll;
    float RawX;
    float RawY;
    float RawZ;
};
#pragma pack(pop)

HANDLE hMapFile = NULL;
FTData* pFTData = nullptr;
bool g_running = true;

const char INDEX_HTML[] = R"rawhtml(<!DOCTYPE html>
<html lang="id">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0, user-scalable=no">
    <title>ETS2 Head Tracker</title>
    <script src="https://cdn.jsdelivr.net/npm/@mediapipe/camera_utils/camera_utils.js" crossorigin="anonymous"></script>
    <script src="https://cdn.jsdelivr.net/npm/@mediapipe/face_mesh/face_mesh.js" crossorigin="anonymous"></script>
    <style>
        * { box-sizing: border-box; }
        body { font-family: sans-serif; background: #ffffff; color: #121212; padding: 15px; margin: 0; text-align: center; }
        h2 { margin: 10px 0; color: #00adb5; font-size: 20px; }
        .cam-container { position: relative; width: 100%; max-width: 360px; height: 220px; margin: 0 auto 15px auto; background: #000; border-radius: 12px; overflow: hidden; border: 2px solid #00adb5; }
        #webcam { width: 100%; height: 100%; object-fit: cover; transform: scaleX(-1); }
        .card { background: #f0f0f0; padding: 15px; border-radius: 10px; margin: 0 auto 15px auto; max-width: 360px; box-shadow: 0 4px 6px rgba(0,0,0,0.1); }
        .btn { background: #00adb5; color: white; border: none; padding: 12px; width: 100%; border-radius: 8px; font-weight: bold; margin-top: 8px; font-size: 14px; cursor: pointer; transition: 0.2s;}
        .btn-reset { background: #393e46; }
        .btn-stop { background: #ff2e63; }
        .val-grid { display: flex; justify-content: space-around; font-family: monospace; font-size: 14px; margin-top: 5px; color: #007a80; font-weight: bold; }
        #status { font-size: 13px; color: #444; margin-bottom: 5px; font-weight: bold; }
    </style>
</head>
<body>
    <h2>ETS2 Head Tracker</h2>
    <div class="cam-container">
        <video id="webcam" playsinline></video>
    </div>
    <div class="card">
        <div id="status">Status: Standby</div>
        <button class="btn" onclick="startAITracking()">1. Aktifkan AI Camera</button>
        <button class="btn btn-reset" onclick="centerFace()">2. Kalibrasi (0,0)</button>
        <button class="btn btn-stop" id="btn-toggle" onclick="toggleTracking()">3. Start Tracking</button>
    </div>
    <div class="card">
        <div class="val-grid">
            <div>Y: <b id="val-yaw">0.0</b></div>
            <div>P: <b id="val-pitch">0.0</b></div>
            <div>R: <b id="val-roll">0.0</b></div>
            <div>Z: <b id="val-z">0.0</b></div>
        </div>
    </div>
    <script>
        let tracking = false;
        let rawYaw = 0, rawPitch = 0, rawRoll = 0, rawZ = 0;
        let smoothYaw = 0, smoothPitch = 0, smoothRoll = 0, smoothZ = 0;
        let offsetYaw = 0, offsetPitch = 0, offsetRoll = 0, offsetZ = 0;
        let baseEyeDist = 0;
        
        const alpha = 0.05; 
        const sensYaw = 2.0;   
        const sensPitch = 5.0; 
        const sensZ = 1500.0;

        let videoElement = document.getElementById('webcam');

        const faceMesh = new FaceMesh({locateFile: (file) => `https://cdn.jsdelivr.net/npm/@mediapipe/face_mesh/${file}`});
        faceMesh.setOptions({ maxNumFaces: 1, refineLandmarks: false, minDetectionConfidence: 0.6, minTrackingConfidence: 0.6 });
        faceMesh.onResults(onResults);

        function onResults(results) {
            if (!results.multiFaceLandmarks || results.multiFaceLandmarks.length === 0) return;
            document.getElementById('status').innerText = tracking ? "Status: Transmitting Data..." : "Status: AI Ready";

            const lm = results.multiFaceLandmarks[0];
            const nose = lm[1], leftEye = lm[33], rightEye = lm[263];
            const eyeCenterDist = (rightEye.x - leftEye.x);
            
            if (baseEyeDist === 0) baseEyeDist = eyeCenterDist;
            
            rawYaw = ((nose.x - leftEye.x) / eyeCenterDist - 0.5) * 150; 
            rawPitch = ((nose.y - ((leftEye.y + rightEye.y) / 2)) * 200) - 30;
            rawRoll = Math.atan2(rightEye.y - leftEye.y, rightEye.x - leftEye.x) * (180 / Math.PI);
            rawZ = (baseEyeDist - eyeCenterDist); 

            smoothYaw = (alpha * rawYaw) + ((1 - alpha) * smoothYaw);
            smoothPitch = (alpha * rawPitch) + ((1 - alpha) * smoothPitch);
            smoothRoll = (alpha * rawRoll) + ((1 - alpha) * smoothRoll);
            smoothZ = (alpha * rawZ) + ((1 - alpha) * smoothZ);

            let finalYaw = ((smoothYaw - offsetYaw) * sensYaw).toFixed(2);
            let finalPitch = ((smoothPitch - offsetPitch) * sensPitch).toFixed(2);
            let finalRoll = (smoothRoll - offsetRoll).toFixed(2);
            let finalZ = ((smoothZ - offsetZ) * sensZ).toFixed(2);

            document.getElementById('val-yaw').innerText = finalYaw;
            document.getElementById('val-pitch').innerText = finalPitch;
            document.getElementById('val-roll').innerText = finalRoll;
            document.getElementById('val-z').innerText = finalZ;

            if (tracking) {
                fetch('/api', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify({ 
                        yaw: parseFloat(finalYaw), 
                        pitch: parseFloat(finalPitch), 
                        roll: parseFloat(finalRoll),
                        z: parseFloat(finalZ)
                    })
                }).catch(() => {});
            }
        }

        async function startAITracking() {
            const camera = new Camera(videoElement, {
                onFrame: async () => { await faceMesh.send({image: videoElement}); },
                width: 480, height: 360
            });
            camera.start();
        }

        function centerFace() { 
            offsetYaw = smoothYaw; 
            offsetPitch = smoothPitch; 
            offsetRoll = smoothRoll; 
            offsetZ = smoothZ;
            baseEyeDist = (rawZ / sensZ) + baseEyeDist; 
        }

        function toggleTracking() {
            tracking = !tracking;
            let btn = document.getElementById('btn-toggle');
            btn.innerText = tracking ? "Stop Tracking" : "Start Tracking";
            btn.style.background = tracking ? "#ff2e63" : "#00adb5";
        }
    </script>
</body>
</html>)rawhtml";

float ExtractJsonValue(const std::string& body, const std::string& key) {
    size_t pos = body.find(key);
    if (pos == std::string::npos) return 0.0f;
    pos += key.length();
    while (pos < body.length() && (body[pos] == ' ' || body[pos] == ':')) pos++;
    try {
        return std::stof(body.substr(pos));
    } catch (...) {
        return 0.0f;
    }
}

void HttpServerThread() {
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    SOCKET serverSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(47991);

    bind(serverSock, (sockaddr*)&serverAddr, sizeof(serverAddr));
    listen(serverSock, SOMAXCONN);

    char buffer[4096];

    while (g_running) {
        SOCKET clientSock = accept(serverSock, NULL, NULL);
        if (clientSock == INVALID_SOCKET) continue;

        int bytesReceived = recv(clientSock, buffer, sizeof(buffer) - 1, 0);
        if (bytesReceived > 0) {
            buffer[bytesReceived] = '\0';
            std::string req(buffer);

            if (req.rfind("GET /", 0) == 0) {
                std::string res = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: " +
                                  std::to_string(sizeof(INDEX_HTML) - 1) + "\r\n\r\n" + INDEX_HTML;
                send(clientSock, res.c_str(), (int)res.length(), 0);
            } 
            else if (req.rfind("POST /api", 0) == 0) {
                if (pFTData) {
                    pFTData->Yaw   = ExtractJsonValue(req, "\"yaw\"");
                    pFTData->Pitch = ExtractJsonValue(req, "\"pitch\"");
                    pFTData->Roll  = ExtractJsonValue(req, "\"roll\"");
                    pFTData->Z     = ExtractJsonValue(req, "\"z\"");
                    pFTData->DataID++;
                }

                std::string res = "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n\r\n{\"status\":\"ok\"}";
                send(clientSock, res.c_str(), (int)res.length(), 0);
            }
        }
        closesocket(clientSock);
    }

    closesocket(serverSock);
    WSACleanup();
}

int main() {
    hMapFile = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(FTData), "FT_SharedMem");
    if (!hMapFile) {
        std::cout << "Gagal create shared memory" << std::endl;
        return 1;
    }

    pFTData = (FTData*)MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(FTData));
    if (!pFTData) {
        CloseHandle(hMapFile);
        std::cout << "Gagal map view of file" << std::endl;
        return 1;
    }

    memset(pFTData, 0, sizeof(FTData));

    std::thread serverThread(HttpServerThread);
    serverThread.detach();

    std::cout << "Server jalan di port 47991. Buka http://localhost:47991" << std::endl;
    std::cout << "Tekan ENTER buat stop program..." << std::endl;
    std::cin.get();

    g_running = false;
    UnmapViewOfFile(pFTData);
    CloseHandle(hMapFile);

    return 0;
}
