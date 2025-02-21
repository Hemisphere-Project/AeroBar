const midi = require('midi');
const osc = require('osc');

// Configuration mapping between specific MIDI events and OSC messages
const config = {
    noteOn: {
        41: {path: '/cues/selected/scenes/by_cell/col_2', value: () => {return 1}},
        42: {path: '/cues/selected/scenes/by_cell/col_3', value: () => {return 1}},
        40: {path: '/cues/selected/scenes/by_cell/col_1', value: () => {return 1}},
        43: {path: '/cues/selected/scenes/by_cell/col_4', value: () => {return 1}},
        36: {path: '/cues/selected/scenes/by_cell/col_5', value: () => {return 1}},
        37: {path: '/cues/selected/scenes/by_cell/col_6', value: () => {return 1}},
        38: {path: '/cues/selected/scenes/by_cell/col_7', value: () => {return 1}},
        39: {path: '/cues/selected/scenes/by_cell/col_8', value: () => {return 1}},
    },
    controlChange: {
        16: {path: '/cues/selected/scenes/by_cell/col_9', value: () => {return 1}},
        17: {path: '/cues/selected/scenes/by_cell/col_10', value: () => {return 1}},
        18: {path: '/cues/selected/scenes/by_cell/col_11', value: () => {return 1}},
        19: {path: '/cues/selected/scenes/by_cell/col_12', value: () => {return 1}},
        12: {path: '/cues/selected/scenes/by_cell/col_13', value: () => {return 1}},
        13: {path: '/cues/selected/scenes/by_cell/col_14', value: () => {return 1}},
        14: {path: '/cues/selected/scenes/by_cell/col_15', value: () => {return 1}},
        15: {path: '/cues/selected/scenes/by_cell/col_16', value: () => {return 1}},

        //70: '/master/master_level',
        //71: '/master/master_video_level',
        73: {path: '/master/master_dmx_level',  value(d) {return d/127.0}},
        77: {path: '/master/engine_speed',      value(d) {return d/64.0}},
    },
    programChange: {
        4: {path: '/cues/selected/scenes/by_cell/col_17', value() {return 1}},
        5: {path: '/cues/selected/scenes/by_cell/col_18', value() {return 1}},
        6: {path: '/cues/selected/scenes/by_cell/col_19', value() {return 1}},
        7: {path: '/cues/selected/scenes/by_cell/col_20', value() {return 1}},
        0: {path: '/cues/selected/scenes/by_cell/col_21', value() {return 1}},
        1: {path: '/cues/selected/scenes/by_cell/col_22', value() {return 1}},
        2: {path: '/cues/selected/scenes/by_cell/col_23', value() {return 1}},
        3: {path: '/cues/selected/scenes/by_cell/col_24', value() {return 1}}    
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
    let oscValue;

    switch (status & 0xf0) {
        case 0x90: // Note On
            if (config.noteOn[data1]) {
                oscAddress = config.noteOn[data1].path;
                oscValue = config.noteOn[data1].value(data2);
            }
            console.log(`Note On: ${data1} Velocity: ${data2}`);
            break;
        case 0x80: // Note Off
            //oscAddress = config.noteOff[data1];
            console.log(`Note Off: ${data1} Velocity: ${data2}`);
            break;
        case 0xb0: // Control Change
            // oscAddress = config.controlChange[data1];
            if (config.controlChange[data1]) {
                oscAddress = config.controlChange[data1].path;
                oscValue = config.controlChange[data1].value(data2);
            }
            console.log(`Control Change: ${data1} Value: ${data2}`);
            break;
        case 0xc0: // Program Change
            // oscAddress = config.programChange[data1];
            if (config.programChange[data1]) {
                oscAddress = config.programChange[data1].path;
                oscValue = config.programChange[data1].value();
            }
            console.log(`Program Change: ${data1}`);
            break;
        default:
            return;
    }

    if (oscAddress && udp.ready) {
        // Send OSC message
        udp.send({
            address: oscAddress,
            args:  [{ type: "f", value: oscValue }]
        });
        console.log(`Sent OSC message: ${oscAddress} ${oscValue}`);
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


