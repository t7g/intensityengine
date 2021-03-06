// Copyright 2010 quaker66. All rights reserved.
// This file is part of Syntensity/the Intensity Engine, an open source project. See COPYING.txt for licensing.

//! This is a flashlight plugin for Syntensity. It's quite heavily based on Firing.js, but a lot modified.
//! Needs Firing and Editing included. Also GameManager, but that's included in every map.
//! It consists of two plugins meant to be activated from your mapscript - protocol and player. Protocol plugin handles server-side stuff,
//! player plugin handles properties, charging and stuff..
//!
//! Besides those two, there are a few functions outside plugins. handleClient effect does visual stuff. doServerRequest sends network message.
//! Keyboard is handled by player plugin and you don't need to care about it.
//! If you want to use it, just make beginning of your player registerEntityClass look like this:
//!
//!  GamePlayer = registerEntityClass(
//!      bakePlugins(
//!          Player,
//!          [
//!              Flashlight.plugins.protocol,
//!              Flashlight.plugins.player,
//!
//! If you want to modify some properties or functions, you use:
//!
//!              Flashlight.plugins.player,
//!              {
//!                  propertyName: 256,
//!                  functionName: function(bleh) {
//!                      do_something();
//!                  },
//!              },
//!
//! Plugin supports two lights, for cars for example. To disable looking up/down with lights,
//! don't forget to force pitch for your car camera.
//! The distance between them has its own variable.
//!
//! If you need full list of modifeable properties, scroll down, it's documented in player plugin.

//! these are required libraries to include, since we use functions from them.
Library.include('library/' + Global.LIBRARY_VERSION + '/Firing');
Library.include('library/' + Global.LIBRARY_VERSION + '/Editing');

Flashlight = {
    plugins: {
        //! Client effect handler, in our case it shows some light.
        handleClientEffect: function(flashlightPlayer, targetPosition, minLightRadius, maxLightRadius, lightColor, carLightsDistance, targetPositionB, targetPositionC, maxDistance, fullLightDistance) {
            // check distances .. based on that, scale light as needed. Don't scale it below minLightRadius or above maxLightRadius
            if (distance(flashlightPlayer.position, targetPosition) > maxLightRadius)
              var dist = maxLightRadius;
            else if (distance(flashlightPlayer.position, targetPosition) < minLightRadius)
              var dist = minLightRadius;
            else
              var dist = distance(flashlightPlayer.position, targetPosition);
            // we want to adjust how much light we want with distance .. we need to get V value of HSV value of color.
            if (fullLightDistance >= 0) { // don't do the conversions to save performance when fullLightDistance is lower than 0, because that means we want to allways light fully.
                var rgb = Color.hexToRgb(lightColor);
                var hsv = Color.rgbToHsv(rgb.r, rgb.g, rgb.b);
            } else var hsv = {h:0, s:0, v:0}; // hey bro, don't crash
            // now we do some range
            if (fullLightDistance < 0) {
                hsv.v = hsv.v; // dont do anything if fullLightDistance is lower than 0, because that means we want full light always.
            } else if (fullLightDistance > 0) { // and if the fullLightDistance is bigger than 0, it means that we want to block fade on some distance .. if it's 0, it means we want to fade out always.
                if (distance(flashlightPlayer.position, targetPosition) <= fullLightDistance)
                    hsv.v = hsv.v; // if the distance is lower or fullLightDistance, don't do anything .. just continue.
                else if (distance(flashlightPlayer.position, targetPosition) <= maxDistance) { // but if we get out of that distance, we need to start fading from that distance
                    hsv.v = -(((hsv.v / (maxDistance - fullLightDistance)) * (distance(flashlightPlayer.position, targetPosition) - fullLightDistance)) - hsv.v);
                    if (hsv.v < 0.005) hsv.v = 0; // a little hack .. because if the value is lower than this, it lights fully for no reason .. the little range between 0 and 0.005 is not noticeable at all.
                } else
                    hsv.v = 0;
            } else if (distance(flashlightPlayer.position, targetPosition) <= maxDistance) { // this is normal, always-fading state.
                hsv.v = -(((hsv.v / maxDistance) * distance(flashlightPlayer.position, targetPosition)) - hsv.v);
                if (hsv.v < 0.005) hsv.v = 0;
            } else { // this is out of maxDistance.
                hsv.v = 0;
            }

            // we convert stuff back to rgb and then hex
            if (fullLightDistance >= 0) { // don't do the conversions to save performance when fullLightDistance is lower than 0, because that means we want to allways light fully.
                var rgb = Color.hsvToRgb(hsv.h, hsv.s, hsv.v); // we convert the number back to insane =)
                var hex = Color.rgbToHex(rgb.r, rgb.g, rgb.b);
            }
            // add our light, finally. Don't show if we're in edit mode.
            if (!isPlayerEditing(flashlightPlayer) && (fullLightDistance < 0 || hsv.v > 0)) // performance - check for hsv.v if it's bigger than 0, because we don't want to spawn invisible light. also when fullLightDistance is lower than 0, spawn light(s).
              if (carLightsDistance < 0) Effect.addDynamicLight(targetPosition, dist, hex ? hex : lightColor, 0);
              else { // car lights - special occasion
                  Effect.addDynamicLight(targetPositionB, dist, hex ? hex : lightColor, 0);
                  Effect.addDynamicLight(targetPositionC, dist, hex ? hex : lightColor, 0);
              }
        },

        //! this is called every frame, so actions can be in sync every frame
        doServerRequest: function(flashlightPlayer, targetPosition, minLightRadius, maxLightRadius, lightColor, carLightsDistance, targetPositionB, targetPositionC, maxDistance, fullLightDistance) {
            // Send network protocol message
            flashlightPlayer.lightingInfo = [targetPosition.x, targetPosition.y, targetPosition.z, minLightRadius, maxLightRadius, lightColor, carLightsDistance, targetPositionB.x, targetPositionB.y, targetPositionB.z, targetPositionC.x, targetPositionC.y, targetPositionC.z, maxDistance, fullLightDistance];
        },

        //! Network protocol so it can be all sync between clients
        protocol: {
            // this is array where we store the target position for clients, has 3 elements (x,y,z)
            lightingInfo: new StateArrayFloat({ clientSet: true, hasHistory: false }),

            // this gets called on server.
            activate: function() {
                this.connect('onModify_lightingInfo', this.onLightingInfo);
            },

            // and this on client.
            clientActivate: function() {
                this.connect('client_onModify_lightingInfo', this.onLightingInfo);
            },

            // finally function doing something visually (calls client effect)
            onLightingInfo: function(info) {
                if (info.length !== 15) return;
                var targetPosition = new Vector3(info.slice(0,3));
                var minLightRadius = info[3];
                var maxLightRadius = info[4];
                var lightColor = info[5];
                var carLightsDistance = info[6];
                var targetPositionB = new Vector3(info.slice(7,10));
                var targetPositionC = new Vector3(info.slice(10,13));
                var maxDistance = info[13];
                var fullLightDistance = info[14];
                if (Global.CLIENT) { // under every client, onLightingInfo gets called and visual effect is shown.
                    Flashlight.plugins.handleClientEffect(this, targetPosition, minLightRadius, maxLightRadius, lightColor, carLightsDistance, targetPositionB, targetPositionC, maxDistance, fullLightDistance); // targetPosition is different under every client.
                }
            },
        },

        //! Player plugin. Takes care of HUD and flashlight battery control.
        player: {
            // counter used to take care of battery charge
            lightCounter: 0,
            // actionKey index used to turn flashlight on/off
            flashlightKeyIndex: 17,
            // is actionkey locked for current player? needed so it's not activating key multiple times for current player.
            actionKeyLocked: 0,
            // the total battery capacity
            lightTime: 30,
            // color of flashlight beam
            lightColor: 0x7b7465,
            // minimal radius. (light gets smaller as we get closer to wall)
            minLightRadius: 3,
            // maximal radius.
            maxLightRadius: 50,
            // maximal distance from player where light still lights (almost invisible, but lights)
            maxDistance: 400,
            // maximal distance where light intensity is kept unchanged. set to 0 if you want to disable it, and -1 if you want to light fully always.
            fullLightDistance: 150,
            // X-left position of HUD
            hudX1: 0.05,
            // Y-bottom position of HUD
            hudY1: 0.9,
            // X-right position of HUD
            hudX2: 0.266,
            // Y-top position of HUD
            hudY2: 0.925,

            // Car lights:
            // distance between lights
            carLightsDistance: 25,
            // enable them?
            carLightsEnabled: 0,

            // function to do HUD, meant to be replaced. By default, show a progressbar changing color =)
            // not really meant for big lightTimes. Present just as example, you'll have your own implementation propably.
            doHUD: function(count) {
               if (CAPI.showHUDRect) {
                var backcolor = (256 * 256 * (parseInt(count * (255 / this.lightTime)) - 255) * (-1)) + 256 * parseInt(count * (255 / this.lightTime));
                CAPI.showHUDRect(this.hudX1 - 0.003, this.hudY1 - 0.005, this.hudX2 + 0.003, this.hudY2 + 0.005, 0xFFFFFF, 0.1);
                CAPI.showHUDRect(this.hudX1, this.hudY1, this.hudX2, this.hudY2, backcolor, 0.4);
                if ((this.hudX1 + (count / (this.lightTime / (this.hudX2 - this.hudX1)))) <= (this.hudX1 + 0.003))
                    var x2 = this.hudX1 + (count / (this.lightTime / (this.hudX2 - this.hudX1))) + 0.003;
                else
                    var x2 = this.hudX1 + (count / (this.lightTime / (this.hudX2 - this.hudX1))) - 0.003;
                CAPI.showHUDRect(this.hudX1 + 0.003, this.hudY1 + 0.005, x2, this.hudY2 - 0.005, 0xFFFFFF, 0.3);
               }
            },

            flashlightOff: function() {
                if (!this.carLightsEnabled) Sound.play("olpc/AdamKeshen/BeatBoxCHIK.wav", this.position);
            },

            flashlightOn: function() {
                if (!this.carLightsEnabled) Sound.play("olpc/AdamKeshen/BeatBoxCHIK.wav", this.position);
            },

            // in clientActivate, we begin stuff .. by default, enable the action key (if not locked), make flashlight off and add charging / discharging mechanism which does something every second.
            clientActivate: function() {
                if (!this.actionKeyLocked) {
                    ApplicationManager.instance.connect('actionKey', bind(this.actionKey, this));
                    this.actionKeyLocked = 1;
                }
                this.flashlightActivated = false;
                if (!this.eventman) {
                   this.eventman = GameManager.getSingleton().eventManager.add({
                        secondsBefore: 0,
                        secondsBetween: 1,
                        func: bind(function() {
                            if (!isPlayerEditing(this) && !this.carLightsEnabled) { // in edit mode, disallow all modifications of counter.
                                if (this.flashlightActivated) {
                                    this.lightCounter += 1;
                                    if (this.lightCounter >= this.lightTime) { this.lightCounter = this.lightTime; this.flashlightDeactivate(); }
                                } else if (this.lightCounter > 0) this.lightCounter -= 1;
                            }
                        }, this),
                        entity: this,
                    });
                }
            },

            // this is function repeated every frame. Here, we get the target and send server request.
            clientAct: function(seconds) {
                if (this !== getPlayerEntity()) return;

                if (this.flashlightActivated) {
                    if (this.lightCounter <= this.lightTime) {
                        var posCopy = this.position.copy();
                        posCopy.z += this.upperHeight;
                        var posCopyB = this.position.copy();
                        posCopyB.z += this.eyeHeight;

                        if (!this.carLightsEnabled) {
                            var autoTargetDirection = posCopy.subNew(posCopyB).normalize();
                            var currentOriginPosition = autoTargetDirection.copy();
                            currentOriginPosition.add(this.position);
                            currentOriginPosition.z += this.upperHeight;

                            var targetData = Firing.findTarget(this, 
                                                               currentOriginPosition,
                                                               posCopyB,
                                                               this.position.addNew(new Vector3().fromYawPitch(this.yaw, this.pitch)),
                                                               Editing.getWorldSize());
                            // include some more info in request, like colors or radiuses.
                            Flashlight.plugins.doServerRequest(this, targetData.target, this.minLightRadius, this.maxLightRadius, this.lightColor, -1, new Vector3(0, 0, 0), new Vector3(0, 0, 0), this.maxDistance, this.fullLightDistance);
                        } else {
                            var posCopyC = posCopyB.copy();
                            var posCopyD = posCopyB.copy();
                            posCopyC.add(new Vector3().fromYawPitch(this.yaw - 90, 0).mul(this.carLightsDistance * -1));
                            posCopyD.add(new Vector3().fromYawPitch(this.yaw + 90, 0).mul(this.carLightsDistance * -1));

                            var autoTargetDirection = posCopy.subNew(posCopyB).normalize();
                            var currentOriginPosition = autoTargetDirection.copy();
                            currentOriginPosition.add(this.position);
                            currentOriginPosition.z += this.upperHeight;

                            var autoTargetDirectionB = posCopy.subNew(posCopyC).normalize();
                            var currentOriginPositionB = autoTargetDirectionB.copy();
                            currentOriginPositionB.add(this.position);
                            currentOriginPositionB.z += this.upperHeight;

                            var autoTargetDirectionC = posCopy.subNew(posCopyD).normalize();
                            var currentOriginPositionC = autoTargetDirectionC.copy();
                            currentOriginPositionC.add(this.position);
                            currentOriginPositionC.z += this.upperHeight;
                    
                            var targetData = Firing.findTarget(this, 
                                                               currentOriginPosition,
                                                               posCopyB,
                                                               this.position.addNew(new Vector3().fromYawPitch(this.yaw, 0)),
                                                               Editing.getWorldSize());

                            var targetDataB = Firing.findTarget(this, 
                                                               currentOriginPositionB,
                                                               posCopyC,
                                                               this.position.addNew(new Vector3().fromYawPitch(this.yaw, 0)),
                                                               Editing.getWorldSize());

                            var targetDataC = Firing.findTarget(this, 
                                                               currentOriginPositionC,
                                                               posCopyD,
                                                               this.position.addNew(new Vector3().fromYawPitch(this.yaw, 0)),
                                                               Editing.getWorldSize());

                            Flashlight.plugins.doServerRequest(this, targetData.target, this.minLightRadius, this.maxLightRadius, this.lightColor, this.carLightsDistance, targetDataB.target, targetDataC.target, this.maxDistance, this.fullLightDistance);
                        }
                    }
                }

                // do HUD stuff here
                if (!this.carLightsEnabled) this.doHUD(this.lightTime - this.lightCounter);
            },

            // we start flashlight. If already started, then stop it.
            flashlightActivate: function() {
                if (this.flashlightActivated) {
                    this.flashlightActivated = false;
                    this.flashlightOff();
                } else {
                    this.flashlightActivated = true;
                    this.flashlightOn();
                }
            },

            // we stop flashlight. No need to start again.
            flashlightDeactivate: function() {
                this.flashlightActivated = false;
                this.flashlightOff();
            },

            // this is our action key. Just starts flashlight if right key is pressed, nothing more.
            actionKey: function(index, down) {
                if (!down) return;
                if (index == this.flashlightKeyIndex) {
                    getPlayerEntity().flashlightActivate();
                }
            },
        },
    },
};
