const midi = require('midi');
const osc = require('osc');

// Configuration mapping between specific MIDI events and OSC messages
const config = {
    noteOn: {
        40: '/cues/selected/scenes/by_cell/col_1',
        41: '/cues/selected/scenes/by_cell/col_2',
        42: '/cues/selected/scenes/by_cell/col_3',
        43: '/cues/selected/scenes/by_cell/col_4',
        36: '/cues/selected/scenes/by_cell/col_5',
        37: '/cues/selected/scenes/by_cell/col_6',
        38: '/cues/selected/scenes/by_cell/col_7',
        39: '/cues/selected/scenes/by_cell/col_8'
    },
    controlChange: {
        16: '/cues/selected/scenes/by_cell/col_9',
        17: '/cues/selected/scenes/by_cell/col_10',
        18: '/cues/selected/scenes/by_cell/col_11',
        19: '/cues/selected/scenes/by_cell/col_12',
        12: '/cues/selected/scenes/by_cell/col_13',
        13: '/cues/selected/scenes/by_cell/col_14',
        14: '/cues/selected/scenes/by_cell/col_15',
        15: '/cues/selected/scenes/by_cell/col_16',
        // 70: '/cues/selected/scenes/by_cell/col_25',
    },
    programChange: {
        4: '/cues/selected/scenes/by_cell/col_17',
        5: '/cues/selected/scenes/by_cell/col_18',
        6: '/cues/selected/scenes/by_cell/col_19',
        7: '/cues/selected/scenes/by_cell/col_20',
        0: '/cues/selected/scenes/by_cell/col_21',
        1: '/cues/selected/scenes/by_cell/col_22',
        2: '/cues/selected/scenes/by_cell/col_23',
        3: '/cues/selected/scenes/by_cell/col_24'
    }
};

// Set up a new input.
const input = new midi.Input();

// OSC port
const udp = new osc.UDPPort({
    localAddress: "0.0.0.0",
    localPort: 8011,
    remoteAddress: "127.0.0.1",
    remotePort: 8010,
    metadata: true
  })
udp.ready = false

// Configure a callback.
input.on('message', (deltaTime, message) => {
    const [status, data1, data2] = message;
    let oscAddress;

    switch (status & 0xf0) {
        case 0x90: // Note On
            oscAddress = config.noteOn[data1];
            console.log(`Note On: ${data1} Velocity: ${data2}`);
            break;
        case 0x80: // Note Off
            oscAddress = config.noteOff[data1];
            console.log(`Note Off: ${data1} Velocity: ${data2}`);
            break;
        case 0xb0: // Control Change
            oscAddress = config.controlChange[data1];
            console.log(`Control Change: ${data1} Value: ${data2}`);
            break;
        case 0xc0: // Program Change
            oscAddress = config.programChange[data1];
            console.log(`Program Change: ${data1}`);
            break;
        default:
            return;
    }

    if (oscAddress && udp.ready) {
        // Send OSC message
        udp.send({
            address: oscAddress,
            args:  [{ type: "i", value: data2 }]
        });
        console.log(`Sent OSC message: ${oscAddress} ${data2}`);
    }
});

// Connect to first available MIDI port
function connectMidi() {
    return new Promise((resolve, reject) => {
        const portCount = input.getPortCount();

        if (portCount === 0) {
            console.log('No MIDI ports available.');
            console.log('Waiting for MIDI device to be connected...');
            setTimeout(() => {
                connectMidi().then(resolve);
            }, 5000);
        } else {
            const portName = input.getPortName(0);
            console.log(`Opening MIDI port: ${portName}`);
            input.openPort(0);
            resolve();
        }
    });
}

// Connect to OSC port
function connectOSC() {
    return new Promise((resolve, reject) => {
        console.log('Opening OSC port...');
        udp.open();
        udp.ready = true
        resolve();
    });
}

connectMidi()
connectOSC()

console.log('Listening for MIDI messages...')

// Also listen for Ctrl+C
process.on('SIGINT', () => {
    console.log('Closing MIDI port...');
    input.closePort();
    console.log('Closing OSC port...');
    udp.close();
    process.exit();
});

udp.on('message', (message) => {
    console.log(message);
});


