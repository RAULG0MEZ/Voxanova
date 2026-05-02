export const PLUGIN_WIDTH = 1360;
export const PLUGIN_HEIGHT = 820;

export const booleanParameters = [
  "peakEnabled",
  "glueEnabled",
  "glueMultiband",
  "faceEnabled",
  "gateEnabled",
  "stereoEnabled",
  "reverbEnabled",
  "reverbSync",
  "reverbDecaySync",
  "reverbPredelaySync",
  "delayEnabled",
  "delaySync",
  "delayPostReverb"
];

export const defaultValues = {
  inputGain: 0,
  outputGain: 0,
  gateThreshold: -80,
  stereoWidth: 0,
  stereoLowBypass: 0,
  preSaturationMode: 0,
  preSaturationAmount: 0,
  postSaturationMode: 0,
  postSaturationAmount: 0,
  peakEnabled: true,
  peakThreshold: 0,
  glueEnabled: true,
  glueMultiband: false,
  glueThreshold: 0,
  glueLowThreshold: 0,
  glueLowMidThreshold: 0,
  glueHighMidThreshold: 0,
  glueAirThreshold: 0,
  faceEnabled: true,
  faceThreshold: 0,
  gateEnabled: true,
  stereoEnabled: false,
  reverbEnabled: false,
  reverbMix: 0,
  reverbDecay: 72,
  reverbSize: 68,
  reverbPredelay: 40,
  reverbLowCut: 0,
  reverbHighCut: 100,
  reverbMode: 0,
  reverbSync: true,
  reverbNoteMode: 0,
  reverbDecaySync: true,
  reverbPredelaySync: true,
  reverbDecayDivision: 2,
  reverbPredelayDivision: 3,
  delayEnabled: false,
  delayMix: 0,
  delayFeedback: 35,
  delayLowCut: 0,
  delayHighCut: 100,
  delaySync: true,
  delayDivision: 2,
  delayNoteMode: 0,
  delayTimeMs: 500,
  delayMode: 0,
  delayPostReverb: false,
  delayStyle: 0
};

export const saturationModes = ["Off", "1073", "Tape", "Tube"];
export const noteModes = ["Note", "Dot", "Triplet"];
export const delayDivisions = ["1/1", "1/2", "1/4", "1/8", "1/16", "1/32", "1/64"];
export const reverbPredelayDivisions = ["None", "1/64", "1/32", "1/16", "1/8", "1/4", "1/2", "1/1"];
export const delayModes = ["Normal", "Wide", "Ping-Pong"];
export const delayStyles = [
  "Clean",
  "Digital",
  "Tape",
  "Studio Tape",
  "Old Tape",
  "Cheap Tape",
  "Analog",
  "Radio",
  "Telephone",
  "Dirty",
  "Ambient"
];

export const reverbModes = [
  "Concert Hall",
  "Bright Hall",
  "Plate",
  "Room",
  "Chamber",
  "Random Space",
  "Chorus Space",
  "Ambience",
  "Sanctuary",
  "Dirty Hall",
  "Dirty Plate",
  "Smooth Plate",
  "Smooth Room",
  "Smooth Random",
  "Nonlin",
  "Chaotic Chamber",
  "Chaotic Hall",
  "Chaotic Neutral",
  "Cathedral",
  "Palace",
  "Chamber1979",
  "Hall1984"
];
