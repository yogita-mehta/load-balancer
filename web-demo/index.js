const express = require('express');
const http = require('http');
const { Server } = require('socket.io');
const { spawn } = require('child_process');
const path = require('path');

const app = express();
const server = http.createServer(app);
const io = new Server(server);

app.use(express.static(path.join(__dirname, 'public')));

io.on('connection', (socket) => {
    console.log('User connected');

    socket.on('start-sim', () => {
        // Path to the compiled binary
        // On Linux/Docker it will be at the root or build folder
        const libPath = path.join(__dirname, '..', 'loadbalancer');
        const configPath = path.join(__dirname, '..', 'config', 'config.json');

        const sim = spawn(libPath, ['simulate', configPath]);

        sim.stdout.on('data', (data) => {
            socket.emit('output', data.toString());
        });

        sim.stderr.on('data', (data) => {
            socket.emit('output', `[ERROR] ${data.toString()}`);
        });

        sim.on('close', (code) => {
            socket.emit('output', `\n[System] Simulation finished with code ${code}`);
        });

        socket.on('disconnect', () => {
            sim.kill();
        });
    });
});

const PORT = process.env.PORT || 3000;
server.listen(PORT, () => {
    console.log(`Web demo running on port ${PORT}`);
});
