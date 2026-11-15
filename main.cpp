#include <stdio.h>
#include <string>
#include <iostream>
#include "core/canvas.h"
#include "core/image.h"
#include "core/scene.h"
#include "core/sprite.h"
#include "core/eventserver.h"

#ifdef BACKEND_X11
    #include "backend/x11backend.h"
	#include "backend/alsabackend.h"
    using SelectedDisplayBackend = X11Backend;
	using SelectedAudioBackend = AlsaBackend;
#elif defined(BACKEND_OTHER)
    #include "backend/otherbackend.h"
    using SelectedDisplayBackend = OtherBackend;
#else
    #error "Define a backend, example: -DBACKEND_X11"
#endif

int main(int argc, char* argv[]) {
	DisplayBackend* displaybackend = new SelectedDisplayBackend();
	displaybackend->init();
	AudioBackend* audiobackend = new SelectedAudioBackend();
	Canvas& canvas = Canvas::getInstance();
	canvas.setCanvasSize(320,240);
	displaybackend->setCanvasImage(canvas.canvas_image);
	EventServer& eventserver = EventServer::getInstance();
	
	Scene scene1;
	if (std::string(argv[1]) == "Sprite") {
		Sprite* sprite = new Sprite();
		sprite->name = std::string(argv[2]);
		Image image;
		image.loadPNG(argv[3]);
		sprite->image = image;
		sprite->x = std::stoi(argv[4]);
		sprite->y = std::stoi(argv[5]);	
		sprite->z = std::stoi(argv[6]);
		scene1.scene_objects.push_back(sprite);
		if (std::string(argv[7]) == "Pressed") {
			sprite->event_type = 1;
		}
		if (std::string(argv[8]) == "setPosition") {
			if (std::string(argv[9]) == "MousePositionX" && std::string(argv[10]) == "MousePositionY") {
				sprite->event_function = [&sprite, &displaybackend, &audiobackend]() 
				{
					sprite->setPosition(
						displaybackend->events_array[0] - (sprite->image.width/2), 
						displaybackend->events_array[1] - (sprite->image.height/2));
					audiobackend->play("audio.wav");
				};				
			}
			else {
				sprite->event_function = [&sprite, x = std::stoi(argv[9]), y = std::stoi(argv[10])]() 
				{
					sprite->setPosition(x, y);
				};
			}
		}
	}
	scene1.run = [&canvas, &displaybackend, &argv](const Event& event) {
		if (event.object->name == std::string(argv[2])) {
				if (event.type == None) {
					if (VisualObject* object = dynamic_cast<VisualObject*>(event.object)) {
						canvas.drawNonPersistentCanvas(object);		
					}	
				}
				if (event.type == event.object->event_type) {
					if (VisualObject* object = dynamic_cast<VisualObject*>(event.object)) {
						object->event_function();
						canvas.drawNonPersistentCanvas(object);		
					}
				}
			}
	};

	while (true) {
		displaybackend->getNextEvent();
		for (int i = 0; i < scene1.scene_objects.size(); i++) {
			Event processed_events = eventserver.processEvents(displaybackend->events_array, scene1.scene_objects[i]);
			scene1.run(processed_events);
		}
	}
    return 0;
}
