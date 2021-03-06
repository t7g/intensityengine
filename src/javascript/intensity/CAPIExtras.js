
// Copyright 2010 Alon Zakai ('kripken'). All rights reserved.
// This file is part of Syntensity/the Intensity Engine, an open source project. See COPYING.txt for licensing.

// Convert vector forms into expanded forms

// Setters

function vectorize(vec) {
    if (vec.x === undefined || vec.y === undefined || vec.z === undefined) {
        eval(assert(' vec.x === undefined && vec.y === undefined && vec.z === undefined '));

        return { x: vec[0], y: vec[1], z: vec[2] };
    } else {
        return vec;
    }
}

CAPI.setExtentO = function(self, vec) {
    vec = vectorize(vec);
    log(DEBUG, format("CAPI.setExtentO: {0},{1},{2}", vec.x, vec.y, vec.z));
    CAPI.setExtentO_raw(self, vec.x, vec.y, vec.z);
};

CAPI.setDynentO = function(self, vec) {
    vec = vectorize(vec);
    log(DEBUG, format("CAPI.setDynentO: {0},{1},{2}", vec.x, vec.y, vec.z));
    CAPI.setDynentO_raw(self, vec.x, vec.y, vec.z);
};

CAPI.setDynentVel = function(self, vec) {
    vec = vectorize(vec);
    CAPI.setDynentVel_raw(self, vec.x, vec.y, vec.z);
};

CAPI.setDynentFalling = function(self, vec) {
    vec = vectorize(vec);
    CAPI.setDynentFalling_raw(self, vec.x, vec.y, vec.z);
};

// Getters

CAPI.getExtentO = function(self) {
    var vec = [ CAPI.getExtentO_raw(self, 0),
                CAPI.getExtentO_raw(self, 1),
                CAPI.getExtentO_raw(self, 2) ];

    log(INFO, format("CAPI.getExtentO: {0}", serializeJSON(vec)));

    return vec;
};

CAPI.getDynentO = function(self, index) {
    return [ CAPI.getDynentO_raw(self, 0),
             CAPI.getDynentO_raw(self, 1),
             CAPI.getDynentO_raw(self, 2) ];
};

CAPI.getDynentVel = function(self, index) {
    return [ CAPI.getDynentVel_raw(self, 0),
             CAPI.getDynentVel_raw(self, 1),
             CAPI.getDynentVel_raw(self, 2) ];
};

CAPI.getDynentFalling = function(self, index) {
    return [ CAPI.getDynentFalling_raw(self, 0),
             CAPI.getDynentFalling_raw(self, 1),
             CAPI.getDynentFalling_raw(self, 2) ];
};


CAPI.setAttachments = function(self, attachments) {
    CAPI.setAttachments_raw(self, attachments.join("|")); // Give Python the format it wants
}

// DEBUG stuff

var oldSetModelName = CAPI.setModelName;

CAPI.setModelName = function(self, model) {
    log(DEBUG, "CAPI.setModelName: " + model + "(" + typeof model + "), self=" + self);
    oldSetModelName(self, model);
};


// Mapping

Map = {
    textureReset: CAPI.textureReset,

    texture: function(type, _name, rot, xoffset, yoffset, scale) {
        rot = defaultValue(rot, 0);
        xoffset = defaultValue(xoffset, 0);
        yoffset = defaultValue(yoffset, 0);
        scale = defaultValue(scale, 1);
        CAPI.texture(type, _name, rot, xoffset, yoffset, scale);
    },

    mapmodelReset: CAPI.mapmodelReset,
    mapmodel: CAPI.mapmodel,

    autograss: CAPI.autograss,

    texLayer: CAPI.texLayer,
    texAlpha: CAPI.texAlpha,
    texColor: CAPI.texColor,
    texFFenv: CAPI.texFFenv,

    setShader: CAPI.setShader,

    setShaderParam: function(_name, x, y, z, w) {
        x = defaultValue(x, 0);
        y = defaultValue(y, 0);
        z = defaultValue(z, 0);
        w = defaultValue(w, 0);
        CAPI.setShaderParam(_name, x, y, z, w);
    },

    materialReset: CAPI.materialReset,

	loadStars: CAPI.loadStars,
    loadSky: CAPI.loadSky,
	loadSun: CAPI.loadSun,
	loadClouds: CAPI.loadClouds,
	loadCloudLayer: CAPI.loadCloudLayer,
	loadAltCloudLayer: CAPI.loadAltCloudLayer,

    fogColor: CAPI.fogColor,
    fog: CAPI.fog,
    shadowmapAngle: CAPI.shadowmapAngle,
    shadowmapAmbient: CAPI.shadowmapAmbient,
    skylight: CAPI.skylight,
    blurSkylight: CAPI.blurSkylight,
    ambient: CAPI.ambient,

	modelTweaks: CAPI.modelTweaks,
	tweakModelAmbient: CAPI.tweakModelAmbient,
	tweakModelGlow: CAPI.tweakModelGlow,
	tweakModelSpec: CAPI.tweakModelSpec,
	tweakModelSpecGlare: CAPI.tweakModelSpecGlare,
	tweakModelGlowGlare: CAPI.tweakModelGlowGlare,
	tweakModelScale: CAPI.tweakModelScale,

	causticScale: CAPI.causticScale,
	causticMillis: CAPI.causticMillis,

	waterSpecularity: CAPI.waterSpecularity,
    waterFog: CAPI.waterFog,
    waterColor: CAPI.waterColor,
	waterFallTint: CAPI.waterFallTint,

	lavaFog: CAPI.lavaFog,
	lavaTint: CAPI.lavaTint,

	spinStars: CAPI.spinStars,
    spinSky: CAPI.spinSky,
	spinSun: CAPI.spinSun,
	spinClouds: CAPI.spinClouds,
	spinCloudLayer: CAPI.spinCloudLayer,
	spinAltCloudLayer: CAPI.spinAltCloudLayer,

	yawStars: CAPI.yawStars,
	yawSky: CAPI.yawSky,
	yawSun: CAPI.yawSun,
	yawClouds: CAPI.yawClouds,
	yawCloudLayer: CAPI.yawCloudLayer,
	yawAltCloudLayer: CAPI.yawAltCloudLayer,

	alphaSky: CAPI.alphaSky,
	alphaSun: CAPI.alphaSun,
	alphaClouds: CAPI.alphaClouds,
	alphaCloudLayer: CAPI.alphaCloudLayer,
	alphaAltCloudLayer: CAPI.alphaAltCloudLayer,

	tintStars: CAPI.tintStars,
	tintSky: CAPI.tintSky,
	tintSun: CAPI.tintSun,
	tintClouds: CAPI.tintClouds,
	tintCloudLayer: CAPI.tintCloudLayer,
	tintAltCloudLayer: CAPI.tintAltCloudLayer,
	tintAmbient: CAPI.tintAmbient,
	tintFog: CAPI.tintFog,

    cloudLayer: CAPI.cloudLayer,
    cloudScrollX: CAPI.cloudScrollX,
    cloudScrollY: CAPI.cloudScrollY,
    cloudScale: CAPI.cloudScale,
	cloudHeight: CAPI.cloudHeight,
	cloudFade: CAPI.cloudFade,
	cloudClip: CAPI.cloudClip,

    altCloudLayer: CAPI.altCloudLayer,
    altCloudScrollX: CAPI.altCloudScrollX,
    altCloudScrollY: CAPI.altCloudScrollY,
    altCloudScale: CAPI.altCloudScale,
	altCloudHeight: CAPI.altCloudHeight,
	altCloudFade: CAPI.altCloudFade,
	altCloudClip: CAPI.altCloudClip,

    skyTexture: CAPI.skyTexture,
    texScroll: CAPI.texScroll,

    preloadSound: function(_name, volume) {
        volume = defaultValue(volume, 100);
        CAPI.preloadSound(_name, volume);
    },

    preloadModel: CAPI.preloadModel,

    convertJP2toPNG: function(src, dest) {
        dest = defaultValue(dest, src.replace('.jp2', '.png'))
        return CAPI.convertJP2toPNG(src, dest);
    },

    convertPNGtoDDS: function(src, dest) {
        dest = defaultValue(dest, src.replace('.png', '.dds'))
        return CAPI.convertPNGtoDDS(src, dest);
    },

    combineImages: CAPI.combineImages,
};

// World

World = {
    isColliding: function(position, radius, ignore) {
        return CAPI.isColliding(position.x, position.y, position.z, radius, ignore ? ignore.uniqueId : -1);
    }
};

World.__defineGetter__('gravity', function () {
    return World._gravity;
});

World.__defineSetter__('gravity', function (value) {
    World._gravity = value;
    CAPI.setGravity(value);
});

//! The gravity to apply to things in this world. The default value,
//! identical to Cube 2, is 200.0. You can change this in your
//! own maps, and even change it dynamically during the game (note
//! that it isn't synchronized between clients and server, so
//! do that manually if you need it).
World.gravity = 200.0; // Sauer default. This sets the value in sauer.

// Libraries

Library = {
    //! Library.include('some/dir/')          - will      load packages/some/dir/Package.js'
    //! Library.include('some/dir/Module')    - will      load packages/some/dir/Module.js'
    //! Library.include('some/dir/Module.js') - will also load packages/some/dir/Module.js'
    //!
    //! Include system: the script is run in eval. This means that
    //!     x = 5;
    //! will be set on the global object, meaning that x is effectively 'exported'
    //! globally. However,
    //!     var x = 5;
    //! will NOT be exported. So, use 'var' to keep things private inside imported scripts.
    //! Note that
    //!     function x() {};
    //! will be treated as if using 'var' by JavaScript - such functions are entirely
    //! local, and NOT exported globally. To export functions globally, use
    //!     x = function() { };
    //! but not that this means x is a nameless function (the name won't appear in
    //! stack traces). To get around that, you can write the (cumbersome)
    //!     x = function x() { };
    //! (i.e., repeat 'x' twice).
    //!
    //! The include system will include a module no more than once. This means it is
    //! ok for each module to include all its dependencies; if they have already
    //! been loaded, nothing will happen.
    //!
    //! @param force If true, the module will be run even if it has been run before.
    //!              This is necessary, for example, if you include the same 'anims.js'
    //!              multiple times for a model's different versions (red/blue, etc.).
    include: function(_name, force) {
        if (_name[_name.length - 1] === '/') {
            _name += 'Package.js';
        } else if (_name.substring(_name.length-3) !== '.js') {
            _name += '.js';
        }

        if (force || !this.loadedModules[_name]) {
            this.loadedModules[_name] = true;
            CAPI.compile(CAPI.readFile(_name), 'Library.include: ' + _name);
        }
    },

    loadedModules: {},
};

/* Version with explicit _export() calls:
//        this.toExport = [];

        eval(CAPI.readFile(_name));

//        forEach(this.toExport, function(item) {
//            eval(format('this.{0} = {0};', item)); // Globalize the item
//        }, this.__global);
//
//        delete this.toExport; // GC it
    },

//    //! Will export the named item (or list of named items) globally.
//    //! This is implemented by saving the exported items and globalizing them
//    //! at the end of the include() call.
//    _export: function(stuff) {
//        if (typeof stuff === 'string') {
//            stuff = [stuff];
//        }
//
//        this.toExport.push.apply(this.toExport, stuff);
//    },
};

//Library.__global = this; // A convenient ref to the global object
*/

// Models

Model = {
    shadow: CAPI.modelShadow,
    collide: CAPI.modelCollide,
    perEntityCollisionBoxes: CAPI.modelPerEntityCollisionBoxes,
    ellipseCollide: CAPI.modelEllipseCollide,

    objLoad: CAPI.objLoad,
    objSkin: CAPI.objSkin,
    objBumpmap: function(meshname, normalmap, skin) {
        skin = defaultValue(skin, '');
        CAPI.objBumpmap(meshname, normalmap, skin);
    },
    objEnvmap: CAPI.objEnvmap,
    objSpec: CAPI.objSpec,
    objPitch: CAPI.objPitch,
    objAmbient: CAPI.objAmbient,
    objGlow: CAPI.objGlow,
    objGlare: CAPI.objGlare,
    objAlphatest: CAPI.objAlphatest,
    objAlphablend: CAPI.objAlphablend,
    objCullface: CAPI.objCullface,
    objFullbright: CAPI.objFullbright,
    objShader: CAPI.objShader,
    objScroll: CAPI.objScroll,
    objNoclip: CAPI.objNoclip,

    alphatest: CAPI.mdlAlphatest,
    bb: function(rad, h, eyeheight) {
        rad = defaultValue(rad, 0.0);
        h = defaultValue(h, 0.0);
        eyeheight = defaultValue(eyeheight, 0.0);
        CAPI.mdlBb(rad, h, eyeheight);
    },
    scale: CAPI.mdlScale,
    spec: CAPI.mdlSpec,
    glow: CAPI.mdlGlow,
    glare: CAPI.mdlGlare,
    ambient: CAPI.mdlAmbient,
    shader: CAPI.mdlShader,
    extendbb: CAPI.mdlExtendbb,
    cullface: CAPI.mdlCullface,
    alphablend: CAPI.mdlAlphablend,
    alphadepth: CAPI.mdlAlphadepth,
    depthoffset: CAPI.mdlDepthoffset,
    fullbright: CAPI.mdlFullbright,
    spin: CAPI.mdlSpin,

    collisionsOnlyForTriggering: CAPI.mdlCollisionsOnlyForTriggering,

    trans: CAPI.mdlTrans,

    md5Dir: CAPI.md5Dir,
    md5Load: CAPI.md5Load,

    md5Skin: function(meshname, tex, masks, envmapmax, envmapmin) {
        masks = defaultValue(masks, '');
        envmapmax = defaultValue(envmapmax, 0);
        envmapmin = defaultValue(envmapmin, 0);
        CAPI.md5Skin(meshname, tex, masks, envmapmax, envmapmin);
    },

    yaw: CAPI.modelYaw,
    pitch: CAPI.modelPitch,

    md2Pitch: CAPI.md2Pitch,
    md2Anim: CAPI.md2Anim,

    md3Load: CAPI.md3Load,
    md3Pitch: CAPI.md3Pitch,
    md3Skin: function(meshname, tex, masks, envmapmax, envmapmin) {
        masks = defaultValue(masks, '');
        envmapmax = defaultValue(envmapmax, 0);
        envmapmin = defaultValue(envmapmin, 0);
        CAPI.md3Skin(meshname, tex, masks, envmapmax, envmapmin);
    },
    md3Spec: CAPI.md3Spec,
    md3Ambient: CAPI.md3Ambient,
    md3Glow: CAPI.md3Glow,
    md3Glare: CAPI.md3Glare,
    md3Alphatest: CAPI.md3Alphatest,
    md3Alphablend: CAPI.md3Alphablend,
    md3Cullface: CAPI.md3Cullface,
    md3Envmap: CAPI.md3Envmap,
    md3Bumpmap: function(meshname, normalmap, skin) {
        skin = defaultValue(skin, '');
        CAPI.md3Bumpmap(meshname, normalmap, skin);
    },
    md3Fullbright: CAPI.md3Fullbright,
    md3Shader: CAPI.md3Shader,
    md3Scroll: CAPI.md3Scroll,
    md3Anim: function(anim, frame, range, speed, priority) {
        speed = defaultValue(speed, 0);
        priority = defaultValue(priority, 0);
        CAPI.md3Anim(anim, frame, range, speed, priority);
    },
    md3Link: CAPI.md3Link,
    md3Noclip: CAPI.md3Noclip,

    md5Bumpmap: function(meshname, normalmap, skin) {
        skin = defaultValue(skin, '');
        CAPI.md5Bumpmap(meshname, normalmap, skin);
    },
    md5Envmap: CAPI.md5Envmap,
    md5Alphatest: CAPI.md5Alphatest,
    md5Alphablend: CAPI.md5Alphablend,

    md5Tag: CAPI.md5Tag,
    md5Anim: function(anim, animfile, speed, priority) {
        speed = defaultValue(speed, 0);
        priority = defaultValue(priority, 0);
        CAPI.md5Anim(anim, animfile, speed, priority);
    },

    md5Animpart: CAPI.md5Animpart,
    md5Pitch: CAPI.md5Pitch,
    md5Adjust: CAPI.md5Adjust,
    md5Spec: CAPI.md5Spec,
    md5Ambient: CAPI.md5Ambient,
    md5Glow: CAPI.md5Glow,
    md5Glare: CAPI.md5Glare,
    md5Cullface: CAPI.md5Cullface,
    md5Fullbright: CAPI.md5Fullbright,
    md5Shader: CAPI.md5Shader,
    md5Scroll: CAPI.md5Scroll,
    md5Link: CAPI.md5Link,
    md5Noclip: CAPI.md5Noclip,

    iqmDir: CAPI.iqmDir,
    iqmLoad: CAPI.iqmLoad,
    iqmTag: CAPI.iqmTag,
    iqmPitch: CAPI.iqmPitch,
    iqmAdjust: CAPI.iqmAdjust,
    iqmSkin: function(meshname, tex, masks, envmapmax, envmapmin) {
        masks = defaultValue(masks, '');
        envmapmax = defaultValue(envmapmax, 0);
        envmapmin = defaultValue(envmapmin, 0);
        CAPI.iqmSkin(meshname, tex, masks, envmapmax, envmapmin);
    },
    iqmSpec: CAPI.iqmSpec,
    iqmAmbient: CAPI.iqmAmbient,
    iqmGlow: CAPI.iqmGlow,
    iqmGlare: CAPI.iqmGlare,
    iqmAlphatest: CAPI.iqmAlphatest,
    iqmAlphablend: CAPI.iqmAlphablend,
    iqmCullface: CAPI.iqmCullface,
    iqmEnvmap: CAPI.iqmEnvmap,
    iqmBumpmap: function(meshname, normalmap, skin) {
        skin = defaultValue(skin, '');
        CAPI.iqmBumpmap(meshname, normalmap, skin);
    },
    iqmFullbright: CAPI.iqmFullbright,
    iqmShader: CAPI.iqmShader,
    iqmScroll: CAPI.iqmScroll,
    iqmAnimpart: CAPI.iqmAnimpart,
    iqmAnim: function(anim, animfile, speed, priority) {
        speed = defaultValue(speed, 0);
        priority = defaultValue(priority, 0);
        CAPI.iqmAnim(anim, animfile, speed, priority);
    },
    iqmLink: CAPI.iqmLink,
    iqmNoclip: CAPI.iqmNoclip,

    smdDir: CAPI.smdDir,
    smdLoad: CAPI.smdLoad,
    smdTag: CAPI.smdTag,
    smdPitch: CAPI.smdPitch,
    smdAdjust: CAPI.smdAdjust,
    smdSkin: function(meshname, tex, masks, envmapmax, envmapmin) {
        masks = defaultValue(masks, '');
        envmapmax = defaultValue(envmapmax, 0);
        envmapmin = defaultValue(envmapmin, 0);
        CAPI.smdSkin(meshname, tex, masks, envmapmax, envmapmin);
    },
    smdSpec: CAPI.smdSpec,
    smdAmbient: CAPI.smdAmbient,
    smdGlow: CAPI.smdGlow,
    smdGlare: CAPI.smdGlare,
    smdAlphatest: CAPI.smdAlphatest,
    smdAlphablend: CAPI.smdAlphablend,
    smdCullface: CAPI.smdCullface,
    smdEnvmap: CAPI.smdEnvmap,
    smdBumpmap: function(meshname, normalmap, skin) {
        skin = defaultValue(skin, '');
        CAPI.smdBumpmap(meshname, normalmap, skin);
    },
    smdFullbright: CAPI.smdFullbright,
    smdShader: CAPI.smdShader,
    smdScroll: CAPI.smdScroll,
    smdAnimpart: CAPI.smdAnimpart,
    smdAnim: function(anim, animfile, speed, priority) {
        speed = defaultValue(speed, 0);
        priority = defaultValue(priority, 0);
        CAPI.smdAnim(anim, animfile, speed, priority);
    },
    smdLink: CAPI.smdLink,
    smdNoclip: CAPI.smdNoclip,

    rdVert: CAPI.rdVert,
    rdEye: CAPI.rdEye,
    rdTri: CAPI.rdTri,
    rdJoint: CAPI.rdJoint,
    rdLimitDist: CAPI.rdLimitDist,
    rdLimitRot: CAPI.rdLimitRot,
    rdAnimJoints: CAPI.rdAnimJoints,

    envmap: CAPI.mdlEnvmap,
};


// User interface

UserInterface = {
    showMessage: CAPI.showMessage,

    showInputDialog: function(content, callback) {
        this.inputDialogCallback = callback;
        CAPI.showInputDialog(content);
    },
};


// Network

Network = {
    connect: CAPI.connect,
};

// Colors
// HSL/HSV functions taken from http://mjijackson.com/2008/02/rgb-to-hsl-and-rgb-to-hsv-color-model-conversion-algorithms-in-javascript

Color = {
    /*
     * Converts an RGB color value to HSL. Conversion formula
     * adapted from http://en.wikipedia.org/wiki/HSL_color_space.
     * Assumes r, g, and b are contained in the set [0, 255] and
     * returns h, s, and l in the set [0, 1].
     */
    rgbToHsl: function(r, g, b) {
        r /= 255, g /= 255, b /= 255;
        var max = Math.max(r, g, b), min = Math.min(r, g, b);
        var h, s, l = (max + min) / 2;

        if(max == min){
            h = s = 0; // achromatic
        }else{
            var d = max - min;
            s = l > 0.5 ? d / (2 - max - min) : d / (max + min);
            switch(max){
                case r: h = (g - b) / d + (g < b ? 6 : 0); break;
                case g: h = (b - r) / d + 2; break;
                case b: h = (r - g) / d + 4; break;
            }
            h /= 6;
        }

        return {h:h, s:s, l:l};
    },

    /*
     * Converts an HSL color value to RGB. Conversion formula
     * adapted from http://en.wikipedia.org/wiki/HSL_color_space.
     * Assumes h, s, and l are contained in the set [0, 1] and
     * returns r, g, and b in the set [0, 255].
     */
    hslToRgb: function(h, s, l) {
        var r, g, b;

        if(s == 0){
            r = g = b = l; // achromatic
        }else{
            function hue2rgb(p, q, t){
                if(t < 0) t += 1;
                if(t > 1) t -= 1;
                if(t < 1/6) return p + (q - p) * 6 * t;
                if(t < 1/2) return q;
                if(t < 2/3) return p + (q - p) * (2/3 - t) * 6;
                return p;
            }

            var q = l < 0.5 ? l * (1 + s) : l + s - l * s;
            var p = 2 * l - q;
            r = hue2rgb(p, q, h + 1/3);
            g = hue2rgb(p, q, h);
            b = hue2rgb(p, q, h - 1/3);
        }

        return {r:(r * 255), g:(g * 255), b:(b * 255)};
    },

    /*
     * Converts an RGB color value to HSV. Conversion formula
     * adapted from http://en.wikipedia.org/wiki/HSV_color_space.
     * Assumes r, g, and b are contained in the set [0, 255] and
     * returns h, s, and v in the set [0, 1].
     */
    rgbToHsv: function(r, g, b) {
        r = r/255, g = g/255, b = b/255;
        var max = Math.max(r, g, b), min = Math.min(r, g, b);
        var h, s, v = max;

        var d = max - min;
        s = max == 0 ? 0 : d / max;

        if(max == min){
            h = 0; // achromatic
        }else{
            switch(max){
                case r: h = (g - b) / d + (g < b ? 6 : 0); break;
                case g: h = (b - r) / d + 2; break;
                case b: h = (r - g) / d + 4; break;
            }
            h /= 6;
        }

        return {h:h, s:s, v:v};
    },

    /*
     * Converts an HSV color value to RGB. Conversion formula
     * adapted from http://en.wikipedia.org/wiki/HSV_color_space.
     * Assumes h, s, and v are contained in the set [0, 1] and
     * returns r, g, and b in the set [0, 255].
     */
    hsvToRgb: function(h, s, v) {
        var r, g, b;

        var i = Math.floor(h * 6);
        var f = h * 6 - i;
        var p = v * (1 - s);
        var q = v * (1 - f * s);
        var t = v * (1 - (1 - f) * s);

        switch(i % 6){
            case 0: r = v, g = t, b = p; break;
            case 1: r = q, g = v, b = p; break;
            case 2: r = p, g = v, b = t; break;
            case 3: r = p, g = q, b = v; break;
            case 4: r = t, g = p, b = v; break;
            case 5: r = v, g = p, b = q; break;
        }

        return {r:(r * 255), g:(g * 255), b:(b * 255)};
    },

    /*
     * Converts a hexadecimal value into RGB value.
     * Accepts integers, don't pass it strings. If you have string, use
     * 
     *     hexToRgb(parseInt(mystring));
     */
    hexToRgb: function(hex) {
        var r, g, b;
        hex = hex.toString(16);

        r = parseInt(hex.substring(0,2),16);
        g = parseInt(hex.substring(2,4),16);
        b = parseInt(hex.substring(4,6),16);

        return {r:r, g:g, b:b};
    },

    /*
     * Converts an RGB array into hexadecimal value.
     */
    rgbToHex: function(r, g, b) {
          var rgb = b | (g << 8) | (r << 16);
        return parseInt('0x' + rgb.toString(16));
    },
};


// New engine stuff

if (!CAPI.renderModelFixed) {
    log(WARNING, "Fixing CAPI.renderModel for version 1 and 2");

    var renderModelOld = CAPI.renderModel;
    var renderModel2Old = CAPI.renderModel2;

    CAPI.renderModel = function() {
        if (!renderModel2Old) {
            // No need to change anything
            renderModelOld.apply(this, arguments);
        } else {
            // CAPI expects new version
            var args = Array.prototype.slice.call(arguments);
            args.splice(8, 0, 0); // Add roll = 0
            renderModel2Old.apply(this, args);
        }
    };

    CAPI.renderModel2 = function() {
        if (!renderModel2Old) {
            // No renderModel2, so downconvert into renderModel
            var args = Array.prototype.slice.call(arguments);
            args.splice(8, 1); // Remove roll
            renderModelOld.apply(this, args);
        } else {
            renderModel2Old.apply(this, arguments);
        }
    };

    CAPI.renderModelFixed = true;
}

// HUD

var oldShowHUDRect = CAPI.showHUDRect;
var oldShowHUDImage = CAPI.showHUDImage;

CAPI.showHUDRect = function(x1, y1, x2, y2, color, alpha) {
    alpha = defaultValue(alpha, 1.0);
    oldShowHUDRect(x1, y1, x2, y2, color, alpha);
};

CAPI.showHUDImage = function(tex, centerx, centery, width, height, color, alpha) {
    color = defaultValue(color, 0xFFFFFF);
    alpha = defaultValue(alpha, 1.0);
    oldShowHUDImage(tex, centerx, centery, width, height, color, alpha);
};

// disable underwater stuff by default ..

CAPI.underwaterAmbient(0);
