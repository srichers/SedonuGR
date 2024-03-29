pipeline {
    triggers { pollSCM('') }  // Run tests whenever a new commit is detected.
    agent { dockerfile {args '--gpus all'}} // Use the Dockerfile defined in the root Flash-X directory
    environment {
	// Get rid of Read -1, expected <someNumber>, errno =1 error
        // See https://github.com/open-mpi/ompi/issues/4948
        OMPI_MCA_btl_vader_single_copy_mechanism = 'none'
    }
    stages {

        //=============================//
    	// Set up submodules and amrex //
        //=============================//
    	stage('Prerequisites'){ steps{
	    sh 'mpicc -v'
	    sh 'git submodule update --init'
	    sh 'cp make.inc.template make.inc'
	    sh 'make nulib'
	}}

	stage('0D SR'){ steps{
	    sh 'make clean; DEBUG=1 NDIMS=0 DO_GR=0 make all -j'
	    sh 'exe/MultiDArrayTest'
	    sh 'make -C tests/inelastic_scatter'
	    // sh 'make -C tests/blackbody'
	}}
	
	stage('1D SR'){ steps{
	    sh 'make clean; DEBUG=1 NDIMS=1 DO_GR=0 make all -j'
	    sh 'make -C tests/spherical_emis'
	    sh 'make -C tests/schwarzschild_path_test'
	    sh 'make -C tests/neutrino_oven'
	    sh 'make -C tests/NSY_regression'
	    sh 'make -C tests/uniform_sphere'
	}}
	
	stage('2D SR'){ steps{
	    sh 'make clean; DEBUG=1 NDIMS=2 DO_GR=0 make all -j'
	}}
	
	stage('3D SR'){ steps{
	    sh 'make clean; DEBUG=1 NDIMS=3 DO_GR=0 make all -j'
	    sh 'make -C tests/3Dbox'
	}}
	
	stage('0D GR'){ steps{
	    sh 'make clean; DEBUG=1 NDIMS=0 DO_GR=1 make all -j'
	    sh 'make -C tests/inelastic_scatter'
	    //sh 'make -C tests/blackbody'
	    sh 'exe/MetricTest'
	}}
	
	stage('1D GR'){ steps{
	    sh 'make clean; DEBUG=1 NDIMS=1 DO_GR=1 make all -j'
	    sh 'make -C tests/spherical_emis'
	    sh 'make -C tests/schwarzschild_path_test'
	    sh 'make -C tests/neutrino_oven'
	    sh 'make -C tests/NSY_regression'
	    sh 'make -C tests/uniform_sphere'
	}}
	
	stage('2D GR'){ steps{
	    sh 'make clean; DEBUG=1 NDIMS=2 DO_GR=1 make all -j'
	}}
	
	stage('3D GR'){ steps{
	    sh 'make clean; DEBUG=1 NDIMS=3 DO_GR=1 make all -j'
	    sh 'make -C tests/3Dbox'
	}}

    } // stages{

    post {
        always {
	    cleanWs(
	        cleanWhenNotBuilt: true,
		deleteDirs: true,
		disableDeferredWipeout: false,
		notFailBuild: true,
		patterns: [[pattern: 'external', type: 'EXCLUDE']] ) // allow amrex to be cached
	}
    }

} // pipeline{
