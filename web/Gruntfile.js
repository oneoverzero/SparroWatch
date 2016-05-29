module.exports = function(grunt){
	grunt.initConfig({
		pkg: grunt.file.readJSON('package.json'),

	    dirs: {
	        output: 'assets',
	    },

	    copy: {
	        dist: {
	            src: '<%= pkg.name %>/dist/css/<%= pkg.name %>.css',
	            dest: '<%= dirs.output %>/css/<%= pkg.name %>-<%= pkg.version %>.css'
	        },
	    },
		}

	);

	// Load the plugin that provides the "copy" task.
	grunt.loadNpmTasks('grunt-contrib-copy');

	// Default task(s).
	grunt.registerTask('default', ['copy']);
};