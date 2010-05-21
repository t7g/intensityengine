
// Copyright 2010 Alon Zakai ('kripken'). All rights reserved.
// This file is part of Syntensity/the Intensity Engine, an open source project. See COPYING.txt for licensing.

Library.include('library/' + Global.LIBRARY_VERSION + '/Firing');


//! Displays messages about headshots
HeadshotPlugin = {
    headshotNotification: new StateInteger({ clientSet: true, hasHistory: false }),

    clientActivate: function() {
        this.connect('client_onModify_headshotNotification', function(value) {
            if (!getEntity(value)) return;

            var shooter = (this === getPlayerEntity()) ? 'You' : this._name;
            var target = (value === getPlayerEntity().uniqueId) ? 'you' : getEntity(value)._name;
            GameManager.getSingleton().addHUDMessage('Headshot! ' + shooter + ' hit ' + target, 0xFF3015, 3.0, 0.63);
            Sound.play('gk/imp_05.ogg');
        });
    },
};

InstaGun = Gun.extend({
    delay: 1.0,
    repeating: false,
    originTag: 'tag_chaingun', // Works with 'stromar' character model; override for others
    damage: 100,

    handleStartLogic: function(shooter, originPosition, targetPosition, targetEntity) {
        this.doRecoil(shooter, 60);

        // damage
        if (targetEntity && targetEntity.sufferDamage) {
            if (!this.maxDamage) {
                this.maxDamage = this.damage; // Set max damage from default
            }
            this.damage = this.maxDamage;

            // Quad damage for headshots. Only players can do headshots, though.
            var aboveEye = targetEntity.aboveEye;
            if (shooter instanceof Player && aboveEye) {
                var neck = CAPI.getAttachmentPosition(targetEntity, 'tag_neck');
                if (targetPosition.z >= neck.z-aboveEye*2 && targetPosition.isCloseTo(neck, aboveEye*2)) {
                    this.damage = 400;
                    shooter.headshotNotification = targetEntity.uniqueId;
                }
            }

            targetEntity.sufferDamage(this);
        }
    },

    handleClientEffect: function(shooter, originPosition, targetPosition, targetEntity) {
        Effect.flare(PARTICLE.STREAK, targetPosition, originPosition, 0.175, 0xFFC864, 0.28);
        Effect.splash(PARTICLE.SMOKE, 3, 0.5, originPosition, 0x222222, 1.4, 50, 501, null, 2, null, 2);
        Effect.addDynamicLight(originPosition, 20, 0xFFFFFF, 0.8, 0.1, 0, 10);
        Effect.splash(PARTICLE.SPARK, 15, 0.1, originPosition, 0xB49B4B, 1.0, 70, 1);
        Effect.splash(PARTICLE.SPARK, 20, 0.1, targetPosition, 0xFFFFFF, 0.1);
        Effect.explodeSplash(PARTICLE.BULLET, targetPosition, 0.05, 0xFFFFFF, 2, 500, 8);

        Effect.flare(
            PARTICLE.STREAK,
            originPosition,
            targetPosition,
            0.1,
            0xFFFFFF
        );
        if (targetEntity && Health.isValidTarget(targetEntity)) {
            Effect.splash(PARTICLE.BLOOD, 13, 1.0, targetPosition, 0x60FFFF, 1.0, 70, 1);
        } else {
            Effect.splash(PARTICLE.SPARK, 15, 0.2, targetPosition, 0xB49B4B, 1.0, 70, 1);
            Effect.addDecal(DECAL.BULLET, targetPosition, originPosition.subNew(targetPosition).normalize(), 3.0);
            Effect.splash(PARTICLE.SMOKE, 3, 0.5, targetPosition, 0x444444, 1.4, 50, 504, null, 2, null, 1);
        }
        Effect.trail(PARTICLE.SMOKE, 0.5, originPosition, targetPosition, 0xC0C0C0, 0.6, 200);
        Effect.trail(0, 0, originPosition, targetPosition, 0, 0, 0, true);
        if (CAPI.getMaterial(shooter.position.x, shooter.position.y, shooter.position.z + 13) === MATERIAL.WATER)
            Sound.play('Q009/uw/st1.ogg', originPosition);
        else
            Sound.play('Q009/re.ogg', originPosition);
        shooter.queueAction(new ShootAction2({ secondsLeft: 1.0 }));
    },
});

/* Not needed
InstaGunPlugin = {
    activate: function() {
        this.attachments.push('*tag_neck');
    },
};
*/

Map.preloadSound('Q009/uw/st1.ogg');
Map.preloadSound('Q009/re.ogg');
