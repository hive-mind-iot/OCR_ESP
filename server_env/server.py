from flask import Flask, request, jsonify
import os
from datetime import datetime
import logging

app = Flask(__name__)

# Configure logging
logging.basicConfig(level=logging.INFO)
logger = logging.getLogger(__name__)

# Configure save directory
UPLOAD_FOLDER = 'uploaded_images'
if not os.path.exists(UPLOAD_FOLDER):
    os.makedirs(UPLOAD_FOLDER)

@app.route('/upload', methods=['POST'])
def upload_image():
    logger.info("Received upload request")
    
    if 'Content-Type' not in request.headers or 'image/jpeg' not in request.headers['Content-Type']:
        logger.warning("Invalid content type: %s", request.headers.get('Content-Type', 'None'))
        return jsonify({'error': 'Content-Type must be image/jpeg'}), 400
    
    try:
        # Get the raw image data
        image_data = request.data
        
        if not image_data:
            logger.warning("No image data received")
            return jsonify({'error': 'No image data received'}), 400
        
        # Create filename with timestamp
        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        filename = f"esp32cam_{timestamp}.jpg"
        filepath = os.path.join(UPLOAD_FOLDER, filename)
        
        # Save the image
        with open(filepath, 'wb') as f:
            f.write(image_data)
        
        image_size = len(image_data)
        logger.info(f"Image saved successfully: {filename} ({image_size} bytes)")
        
        return jsonify({
            'success': True,
            'message': 'Image saved successfully',
            'filename': filename,
            'size': image_size
        }), 200
        
    except Exception as e:
        logger.error(f"Error saving image: {str(e)}")
        return jsonify({'error': str(e)}), 500

@app.route('/status', methods=['GET'])
def status():
    return jsonify({'status': 'Server is running'}), 200

if __name__ == '__main__':
    logger.info(f"Starting server. Images will be saved to: {os.path.abspath(UPLOAD_FOLDER)}")
    # Run on all interfaces on port 5000
    app.run(host='0.0.0.0', port=5000, debug=True)