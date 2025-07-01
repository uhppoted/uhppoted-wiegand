#!python3

"""
Frequency response for input RC filter

cutoff: 50Hz
fs:     1kHz 
"""

from scipy import signal
from matplotlib.ticker import ScalarFormatter

import matplotlib.pyplot as plt
import numpy as np

# 𝑓𝑐=1/2𝜋𝑅𝐶
# 2𝜋𝑅𝐶 = 1/𝑓𝑐
# 𝑅 = 1/𝐶.2𝜋𝑓𝑐

# PCAL6408APW
# Vᴅᴅ   = 5V
# Vʟᴏᴡ  = 0.3Vᴅᴅ (1.5V)
# Vʜɪɢʜ = 0.7Vᴅᴅ (3.5V)
# Vʜ    = 3.5V - 1.5V = 2V

PI = np.pi
N = 1          # order of filter
fs = 1000      # sampling frequency (Hz)
aliasing = 0.4 # max. allowable aliasing noise (0.3Vcc to 0.7Vcc)

C = 10/1000_000_000 # C1 (10nF)
R = 2000             # R2 (1k) + R13 (1k)
f0 = 1/(R*C*2*PI)

print('R:',R)
print('C:',C)
print('f0:',f0)

# Butterworth filter (analog)
b, a = signal.butter(N, 2*PI*f0, 'lowpass', analog=True)
w, h = signal.freqs(b, a)
f = w/(2*PI)

# Plot frequency response (dB)
plt.semilogx(f, 20 * np.log10(abs(h)))
plt.title('Butterworth analog filter frequency response')
plt.xlabel('Frequency [Hz]')
plt.ylabel('Amplitude [dB]')
plt.margins(0, 0.1)
plt.grid(which='both', axis='both')
plt.axvline(f0, color='green')  # cutoff frequency
plt.gca().xaxis.set_major_formatter(ScalarFormatter())
plt.show()

# Plot extended frequency response (voltage)
w, h = signal.freqs(b, a, worN=2*PI*np.logspace(0, 3, num=500, base=10))
f = w/(2*PI)

plt.semilogx(f, abs(h))
plt.title('Butterworth analog filter frequency response')
plt.xlabel('Frequency [Hz]')
plt.ylabel('Amplitude [V]')
plt.margins(0, 0.1)
plt.grid(which='both', axis='both')
plt.axvline(f0, color='green')      # cutoff frequency
plt.axvline(fs/2, color='green')    # Nyquist freqency (internal LPF)
plt.axhline(aliasing, color='red')  # max. allowable aliasing amplitude
plt.gca().xaxis.set_major_formatter(ScalarFormatter())
plt.show()


