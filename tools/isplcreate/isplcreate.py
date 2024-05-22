#!/usr/bin/python3
import re
import argparse
import struct
from pydub import AudioSegment
from intelhex import IntelHex
import zlib

class ISPLManifest:
  def __init__(self):
    self.audioAssets = { }
  

class AudioAssetManifest:
  def __init__(self):
    self.audioAssets = { }
    self.audioAssetIdxs = [ ]

  def add(self, asset):
    if asset.name not in self.audioAssets.keys():
      self.audioAssets[asset.name] = asset
      self.audioAssetIdxs.append(asset.name)
      return self.audioAssetIdxs.index(asset.name)

  def getAssetIdx(self, assetName):
    return self.audioAssetIdxs.index(assetName)

  def getEntrySize(self):
    return 15

  def getEntries(self):
    return len(self.audioAssetIdxs)

  def write(self, startingOffset):
    # We know we have X audio assets, each of which needs a table entry
    # Audio Asset Record Table Entry:
    # [Type:8] [Addr:32] [Size:32] [SampleRate:16] [Flags:32]
    
    # audio asset table len = 15 bytes * num_assets
    assetTableLen = 15 * len(self.audioAssetIdxs)
    
    tableBlob = bytearray()
    audioBlob = bytearray()
    
    for i in range(0,len(self.audioAssetIdxs)):
      asset = self.audioAssets[self.audioAssetIdxs[i]]
      
      typeVal = 0
      if asset.audioType == 'PCM':
        typeVal = 1

      print("Writing audio asset %d [%s] at addr=%d, len=%d, rate=%d" % (i, self.audioAssetIdxs[i], startingOffset + assetTableLen + len(audioBlob), len(asset.data), asset.rate))
      tableRec = struct.pack('>BIIHI', typeVal, startingOffset + assetTableLen + len(audioBlob), len(asset.data), asset.rate, asset.flags)
      tableBlob += tableRec
      audioBlob += asset.data
      
    return tableBlob + audioBlob
    
  
class AudioAsset:
  def __init__(self, name='', audioType='', data=None, rate=0, flags=0):
    self.name = name
    self.audioType = audioType
    self.data = data
    self.rate = rate
    self.flags = flags
    
    
  

def main():
  parser = argparse.ArgumentParser(description='Interface with an Arduino-based SPI flash programmer')
  parser.add_argument('-i', dest='input_directory', default='test',
                      help='input ISPL file')
  parser.add_argument('-o', dest='output_filename', default='flash.bin',
                      help='file to write to')

  args = parser.parse_args()
  
  pcm_asset_match = re.compile(r'AudioFile (?P<assetName>\w+) PCM (?P<sampleRate>\d+) (?P<assetFilename>.+)')
  wav_asset_match = re.compile(r'AudioFile (?P<assetName>\w+) WAV (?P<sampleRate>\d+) (?P<assetFilename>.+)')

  asset_match = re.compile(r'AudioFile (?P<assetName>\w+) PCM (?P<sampleRate>\d+) (?P<assetFilename>.+)')

  hex_match = re.compile(r'Firmware (?P<assetFilename>.+)')
  
  # Compile assets
  
  isplFilename = args.input_directory + '/program.ispl'
  
  audioAssetManifest = AudioAssetManifest()
  assetDict = { }
  with open(isplFilename, 'r+') as isplFile:
    print("Assembling audio assets")

    pcmAssetRecs = filter(lambda e: e is not None, [pcm_asset_match.match(rec) for rec in isplFile.readlines()])
    isplFile.seek(0)
    wavAssetRecs = filter(lambda e: e is not None, [wav_asset_match.match(rec) for rec in isplFile.readlines()])
    
    for assetRec in pcmAssetRecs:
      try:
        with open(args.input_directory + '/' + assetRec.group('assetFilename'), 'rb') as f:
          newAsset = AudioAsset(name=assetRec.group('assetName'), 
            audioType = 'PCM',
            data = f.read(),
            rate = int(assetRec.group('sampleRate')))
          assetDict[newAsset.name] = audioAssetManifest.add(newAsset)
          
          print("- Added PCM asset [%s] - rate=%dHz, bytes=%d len=%.1fs" % (newAsset.name, newAsset.rate, len(newAsset.data), len(newAsset.data) / newAsset.rate))    
      except IOError:
        print("ERROR: Audio asset [%s] - file [%s] cannot be opened" % (assetRec.group('assetName'), assetRec.group('assetFilename')))
        raise IOError

    for assetRec in wavAssetRecs:
      try:
        wavFilename = args.input_directory + '/' + assetRec.group('assetFilename')
        wav = AudioSegment.from_wav(wavFilename)
        # Convert to 8-bit unsigned PCM at prescribed sample rate here
        sampleRate = int(assetRec.group('sampleRate'))
        wav = AudioSegment.from_wav(wavFilename).set_channels(1).set_frame_rate(sampleRate).set_sample_width(1)
        u8data = bytearray()
        for b in wav.raw_data:
          u8data.append(0xFF & (b+0x80))

        newAsset = AudioAsset(name=assetRec.group('assetName'), 
          audioType = 'PCM',
          data = u8data,
          rate = sampleRate)
        assetDict[newAsset.name] = audioAssetManifest.add(newAsset)
          
        print("- Added WAV asset [%s] - rate=%dHz, bytes=%d len=%.1fs" % (newAsset.name, newAsset.rate, len(newAsset.data), len(newAsset.data) / newAsset.rate))    
      except IOError:
        print("ERROR: Audio asset [%s] - file [%s] cannot be opened" % (assetRec.group('assetName'), assetRec.group('assetFilename')))
        raise IOError

    isplFile.seek(0)
    hexAssetRecs = []

    for assetRec in filter(lambda e: e is not None, [hex_match.match(rec) for rec in isplFile.readlines()]):
      hexAssetRecs.append(assetRec)

    if len(hexAssetRecs) > 1:
      print("ERROR: Too many Firmware records, only 1 allowed")
      raise IOError
    elif len(hexAssetRecs) < 1:
      print("ERROR: Missing Firmware record")
      raise IOError

    assetRec = hexAssetRecs[0]
    
    try:
      firmware = IntelHex()
      firmware.fromfile(args.input_directory + '/' + assetRec.group('assetFilename'), format='hex')
      programData = firmware.tobinarray(start=0)
    except IOError:
      print("ERROR: Firmware asset file [%s] cannot be opened" % (assetRec.group('assetFilename')))
      raise IOError

    
    print("- Program is %d bytes long" % (len(programData)))
    programDataCRC = zlib.crc32(programData)
    print("- Program CRC32 is %08X" % (programDataCRC))
    
    programData = bytes(programData) + struct.pack('>I', programDataCRC)
  
  # Build asset table

  # Build manifest
  
  # Write Header
  
  with open(args.output_filename, 'wb') as outfile:
    bytesWritten = 0
    
    headerData = struct.pack('>4sI', 'ISPL'.encode('utf-8'), 1)
    outfile.write(headerData)
    bytesWritten += len(headerData)

    # Hard-wire manifest for now
    # Three records
    # 0 - Manifest itself
    # 1 - Program data
    # 2 - Audio asset table
    # *  Manifest Table Record:
    # *  [Addr:32] [Recs-N:32] [Recs-S:16]
    
    manifestFinalSize = 10 * 3
    
    print("Starting manifest write at address %d" % (bytesWritten))
    
    manifestData = struct.pack('>IIH', bytesWritten, 3, 10)  # manifest self record
    print("Manifest Table 0 - %d %d %d" % (bytesWritten, 3, 10))
    
    manifestData += struct.pack('>IIH', bytesWritten + manifestFinalSize, len(programData), 1)
    print("Manifest Table 1 - %d %d %d" % (bytesWritten + manifestFinalSize, len(programData), 1))
    
    manifestData += struct.pack('>IIH', bytesWritten + manifestFinalSize + len(programData), audioAssetManifest.getEntries(), audioAssetManifest.getEntrySize())
    print("Manifest Table 2 - %d %d %d" % (bytesWritten + manifestFinalSize + len(programData), audioAssetManifest.getEntries(), audioAssetManifest.getEntrySize()))
    
    outfile.write(manifestData)
    bytesWritten += len(manifestData)
    
    
    print("Starting program write at address %d" % (bytesWritten))
    outfile.write(programData)
    bytesWritten += len(programData)
    
    print("Starting audio asset write at address %d" % (bytesWritten))
    outfile.write(audioAssetManifest.write(bytesWritten))
 
def hexDump(byteArray):
  for i in range(0, len(byteArray)):
    if 0 == i%16:
      print("");
    print("%02X " % byteArray[i], end='')

if __name__ == '__main__':
  main()

