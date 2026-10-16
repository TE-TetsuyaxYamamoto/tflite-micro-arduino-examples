#include <PDM.h>

static const int kSampleRate = 16000;
static const uint16_t kSamplesPerBlock = 512;
static const uint8_t kQueueSize = 8;

static int16_t audioQueue[kQueueSize][kSamplesPerBlock];
static uint16_t queuedSamples[kQueueSize];
static uint16_t queuedSequence[kQueueSize];
static volatile uint8_t queueHead = 0;
static volatile uint8_t queueTail = 0;
static uint16_t nextSequence = 0;
static uint8_t discardBuffer[128];

static void discardPdmBytes(int bytesAvailable) {
  while (bytesAvailable > 0) {
    const int bytesToRead = min(bytesAvailable, static_cast<int>(sizeof(discardBuffer)));
    const int bytesRead = PDM.read(discardBuffer, bytesToRead);
    if (bytesRead <= 0) {
      break;
    }
    bytesAvailable -= bytesRead;
  }
}

void onPDMdata() {
  const int bytesAvailable = PDM.available();
  if (bytesAvailable <= 0) {
    return;
  }

  const uint16_t sequence = nextSequence++;
  const int samplesAvailable = bytesAvailable / sizeof(int16_t);
  const uint8_t nextHead = (queueHead + 1) % kQueueSize;
  if ((bytesAvailable % sizeof(int16_t)) != 0 ||
      samplesAvailable > kSamplesPerBlock || nextHead == queueTail) {
    discardPdmBytes(bytesAvailable);
    return;
  }

  const int bytesRead = PDM.read(audioQueue[queueHead], bytesAvailable);
  if (bytesRead != bytesAvailable) {
    return;
  }

  queuedSamples[queueHead] = bytesRead / sizeof(int16_t);
  queuedSequence[queueHead] = sequence;
  queueHead = nextHead;
}

void setup() {
  Serial.begin(115200);
  while (!Serial) {
  }

  PDM.onReceive(onPDMdata);
  if (!PDM.begin(1, kSampleRate)) {
    while (true) {
    }
  }
}

void loop() {
  if (queueTail == queueHead) {
    return;
  }

  const uint8_t tail = queueTail;
  const uint16_t sampleCount = queuedSamples[tail];
  const uint16_t sequence = queuedSequence[tail];
  const uint8_t header[] = {
      'A', 'U', 'D', '0',
      static_cast<uint8_t>(sequence & 0xff),
      static_cast<uint8_t>(sequence >> 8),
      static_cast<uint8_t>(sampleCount & 0xff),
      static_cast<uint8_t>(sampleCount >> 8),
  };

  Serial.write(header, sizeof(header));
  Serial.write(reinterpret_cast<const uint8_t*>(audioQueue[tail]),
               sampleCount * sizeof(int16_t));
  queueTail = (tail + 1) % kQueueSize;
}