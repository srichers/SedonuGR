makeplot = True
if makeplot:
    import matplotlib as mpl
    mpl.use('Agg')
    import matplotlib.pyplot as plt
    import matplotlib.gridspec as gridspec
    mpl.rcParams['font.size'] = 16
    mpl.rcParams['font.family'] = 'serif'
    mpl.rcParams['xtick.major.size'] = 7
    mpl.rcParams['xtick.major.width'] = 2
    mpl.rcParams['xtick.major.pad'] = 8
    mpl.rcParams['xtick.minor.size'] = 4
    mpl.rcParams['xtick.minor.width'] = 2
    mpl.rcParams['ytick.major.size'] = 7
    mpl.rcParams['ytick.major.width'] = 2
    mpl.rcParams['ytick.minor.size'] = 4
    mpl.rcParams['ytick.minor.width'] = 2
    mpl.rcParams['axes.linewidth'] = 2
import h5py
import numpy as np
import sys
sys.path.insert(0, '../')
import tools

avg_tolerance = 0.05
max_tolerance = 0.5

munue = 0
with open("output.txt","r") as search:
    for line in search:
        if "DO_GR" in line:
            do_gr = int(line[-2])

f = h5py.File("fluid_00001.h5","r")
r = np.array(f["axes/x0(cm)[mid]"])/1e5
T_gas = np.array(f["T_gas(K,tet)"])*tools.k_b/tools.MeV
edens = np.array(f["distribution0(erg|ccm,tet)"]).sum(axis=(1,2,3))/1e29
edens_theory = np.array([tools.edens(T_gas[i]*tools.MeV, munue*tools.MeV) for i in range(len(T_gas))])/1e29
#edens_theory = np.array([tools.energy_blackbody(T_gas[i]*tools.MeV, munue*tools.MeV) for i in range(len(T_gas))])
if makeplot:
    fig = plt.figure(figsize=(8,8))
    gs = gridspec.GridSpec(2, 1)
    axes = [plt.subplot(gsi) for gsi in gs]

isError = False

if makeplot:
    axes[0].set_ylabel(r"$E$ ($10^{29}$ erg cm$^{-3}$)")
    axes[0].plot(r, edens_theory, "g-",label="Theory")
    axes[0].scatter(r, edens, label="Sedonu")
    axes[0].legend(frameon=False)
    axes[0].set_ylim(0,1.1*max(np.max(edens),np.max(edens_theory)))
error = edens-edens_theory
relerror = error/edens_theory
avgerror = np.abs(np.mean(relerror))
maxerror = np.max(np.abs(relerror))
print("Energy density avgerror=",avgerror," maxerror=",maxerror)
if avgerror>avg_tolerance or maxerror>max_tolerance:
    isError = True
    
e_abs = np.array(f["four-force[abs](erg|ccm|s,tet)"])[:,3]/1e30
e_emit = np.array(f["four-force[emit](erg|ccm|s,tet)"])[:,3]/1e30
dEdt = e_abs + e_emit
if makeplot:
    axes[1].set_ylabel(r"$dE_\mathrm{int}/dt$ ($10^{30}$ erg cm$^{-3}$ s$^{-1}$)")
    axes[1].plot(r, -e_emit, label="Emit")
    axes[1].scatter(r, e_abs, label="Absorb")
    axes[1].legend(frameon=False)
    axes[1].set_ylim(0,1.1*max(np.max(e_abs),np.max(-e_emit)))
error = dEdt
relerror = error / e_emit
avgerror = np.abs(np.mean(relerror))
maxerror = np.max(np.abs(relerror))
print("dEdt avgerror=",avgerror," maxerror=",maxerror)
if avgerror>avg_tolerance or maxerror>max_tolerance:
    isError = True

#l_abs = np.array(f["l_abs(1|s|ccm,tet)"])/1e35
#l_emit = np.array(f["l_emit(1|s|ccm,tet)"])/1e35
#dNdt = l_abs + l_emit
#if makeplot:
#    axes[2].set_ylabel(r"$dn_l/dt$ ($10^{35}$ cm$^{-3}$ s$^{-1}$)")
#    axes[2].plot(r, -l_emit, label="Emit")
#    axes[2].scatter(r, l_abs, label="Absorb")
#    axes[2].set_ylim(0,1.1*max(np.max(l_abs),np.max(-l_emit)))
#    axes[2].legend(frameon=False)
#error = dNdt
#relerror = dNdt / l_emit
#avgerror = np.abs(np.mean(relerror))
#maxerror = np.max(np.abs(relerror))
#print("dEdt avgerror=",avgerror," maxerror=",maxerror)
#if avgerror>avg_tolerance or maxerror>max_tolerance:
#    isError = True

if makeplot:
    for ax in axes:
        ax.minorticks_on()
        ax.axvline(1.5,color="k",linewidth=0.5)
        ax.tick_params(axis='both',which='both',direction='in',top=True,right=True)
        ax.set_xlim(0,5)
    
    axes[1].set_xlabel("$r$ (km)")
    axes[0].xaxis.set_ticklabels([])
    axes[0].text(1.3,3,r"$1.5\,r_\mathrm{sch}$",rotation=-90)

    plt.subplots_adjust(wspace=0, hspace=0)
    plt.savefig("edens.pdf", bbox_inches="tight")

if isError:
    raise Exception("temperature_redshift results are outside of the tolerance.")
else:
    print("SUCCESS")
