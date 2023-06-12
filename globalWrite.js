const functions = require('firebase-functions');
const admin = require('firebase-admin');
const cors = require('cors')({ origin: 'https://balanzasupersonica.web.app' });
const moment = require('moment');

admin.initializeApp();

exports.globalWrite = functions.https.onRequest(async (request, response) => {
    cors(request, response, async () => {
        try {
            const collection = request.body.collection;
            const weight = request.body.weight;
            const height = request.body.height;

            const db = admin.firestore();
            const now = moment().format('YYYY-MM-DD HH:mm:ss');
            const newDocRef = db.collection(collection).doc(now);

            await newDocRef.set({
                weight: weight,
                height: height
            });

            response.status(200).send(`Weight: ${weight}kg and Height: ${height}cm set in Collection: ${collection}`);
        } catch (error) {
            console.error('Error adding data to Firestore:', error);
            response.set('Access-Control-Allow-Origin', '*');
            response.set('Access-Control-Allow-Methods', 'POST');
            response.set('Access-Control-Allow-Headers', 'Content-Type');
            response.set('Access-Control-Max-Age', '3600');
            response.status(500).send(error);
        }
    });
});