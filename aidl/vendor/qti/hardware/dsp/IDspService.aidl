// SPDX-License-Identifier: BSD-3-Clause-Clear
package vendor.qti.hardware.dsp;
import vendor.qti.hardware.dsp.IDspManager;
interface IDspService { IDspManager getNewDSPSession(int flags); }
