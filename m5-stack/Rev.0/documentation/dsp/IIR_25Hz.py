#!python3

"""
Frequency response scipy 1storder IIR Butterworth filter

cutoff: 25Hz
fs:     1kHz 
"""

from scipy import signal
from matplotlib.ticker import ScalarFormatter

import matplotlib.pyplot as plt
import numpy as np

Vᴅᴅ   = 1
Vʟᴏᴡ  = 0.1
Vʜɪɢʜ = 0.9
Vʜ    = Vʜɪɢʜ - Vʟᴏᴡ

N = 1      # order of filter
fc = 25    # cutoff frequency (Hz)
fs = 1000  # sampling frequency (Hz)
hysteresis = Vʜ # internal software Schmitt trigger points

# IIR Butterworth filter
b, a = signal.butter(N, fc, 'lowpass', analog=False, fs=fs)
w, h = signal.freqz(b, a, fs=fs)

print('a:', a)
print('b:', b)

# Frequency response (dB)
plt.semilogx(w, 20 * np.log10(abs(h)))
plt.title('Butterworth IIR filter')
plt.xlabel('Frequency [Hz]')
plt.ylabel('Amplitude [dB]')
plt.margins(0, 0.1)
plt.grid(which='both', axis='both')
plt.axvline(fc, color='green')  # cutoff frequency
plt.axhline(-3, color='red')  # -3dB
plt.gca().xaxis.set_major_formatter(ScalarFormatter())
plt.show()

# Frequency response (voltage)
w, h = signal.freqz(b, a, fs=fs)

plt.semilogx(w, abs(h))
plt.title('Butterworth IIR filter')
plt.xlabel('Frequency [Hz]')
plt.ylabel('Amplitude [V]')
plt.margins(0, 0.1)
plt.grid(which='both', axis='both')
plt.axvline(fc, color='green')        # cutoff frequency
plt.axvline(fs/2, color='green')      # Nyquist freqency (internal LPF)
plt.axhline(hysteresis, color='red')  # max. allowable aliasing amplitude
plt.gca().xaxis.set_major_formatter(ScalarFormatter())
plt.show()


