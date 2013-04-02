#include <fstream>
#include <memory>
#include <stdexcept>

#include <boost/lexical_cast.hpp>

#include <pugixml.hpp>

#include <spine/SkeletonData.h>

#include <NinjaParty/AssetManager.hpp>
#include <NinjaParty/DeminaAnimationPlayer.hpp>
#include <NinjaParty/Font.hpp>
#include <NinjaParty/GleedLevel.hpp>
#include <NinjaParty/Path.hpp>
#include <NinjaParty/Song.hpp>
#include <NinjaParty/SoundEffect.hpp>
#include <NinjaParty/SpineAnimationPlayer.hpp>
#include <NinjaParty/SpriteAnimationPlayer.hpp>
#include <NinjaParty/Texture.hpp>
#include <NinjaParty/TextureDictionary.hpp>

namespace NinjaParty
{
	AssetManager::AssetManager(const std::string &assetPath)
	{
		this->assetPath = GetRootPath() + assetPath;
	}
	
	template <typename T>
	void DeleteMapContents(std::map<std::string, T> &map)
	{
		auto iterator = map.begin();
		auto end = map.end();
	
		while(iterator != end)
		{
			delete iterator->second;
			++iterator;
		}
	}
	
	AssetManager::~AssetManager()
	{
		DeleteMapContents(songs);
		DeleteMapContents(soundEffects);
		DeleteMapContents(textures);
		DeleteMapContents(textureDictionaries);
		DeleteMapContents(fonts);
		DeleteMapContents(deminaAnimations);
		DeleteMapContents(spriteAnimations);
		DeleteMapContents(spineSkeletons);
		DeleteMapContents(spineAnimations);
	}
	
	Texture* AssetManager::LoadTexture(std::string const &fileName)
	{
		auto iterator = textures.find(fileName);
		
		if(iterator != textures.end())
			return iterator->second;
		
		Texture *texture = Texture::FromFile(assetPath + fileName);
		if(texture == nullptr)
			return nullptr;
		
		textures[fileName] = texture;
		return texture;
	}

	TextureDictionary* AssetManager::LoadTextureDictionary(std::string const &fileName)
	{
		auto iterator = textureDictionaries.find(fileName);
		
		if(iterator != textureDictionaries.end())
			return iterator->second;
		
		TextureDictionary *textureDictionary = TextureDictionary::FromFile(assetPath + fileName);
		if(textureDictionary == nullptr)
			return nullptr;
		
		textureDictionaries[fileName] = textureDictionary;
		return textureDictionary;
	}

	Song* AssetManager::LoadSong(const std::string &fileName)
	{
		auto iterator = songs.find(fileName);
		
		if(iterator != songs.end())
			return iterator->second;
		
		Song *song = new Song(assetPath + fileName);
		
		songs[fileName] = song;
		return song;
	}
	
	SoundEffect* AssetManager::LoadSoundEffect(const std::string &fileName)
	{
		auto iterator = soundEffects.find(fileName);
		
		if(iterator != soundEffects.end())
			return iterator->second;
		
		SoundEffect *soundEffect = new SoundEffect(assetPath + fileName);
		
		soundEffects[fileName] = soundEffect;
		return soundEffect;
	}

	Font* AssetManager::LoadFont(const std::string &fileName)
	{
		std::map<std::string, Font*>::iterator iter = fonts.find(fileName);
		if (iter != fonts.end())
		{
			return iter->second;
		}

		pugi::xml_document document;
		pugi::xml_parse_result result = document.load_file((assetPath + fileName).c_str());
			
		if(!result)
			throw std::runtime_error(std::string("Failed to load font: ") + fileName);

		Font *font = new Font();

		pugi::xml_node rootNode = document.child("font");

		if(!rootNode)
			throw std::runtime_error(std::string("Failed to load font: ") + fileName);

		font->height = rootNode.child("common").attribute("lineHeight").as_int();
		
		pugi::xml_node charsNode = rootNode.child("chars");

		for(pugi::xml_node node = charsNode.child("char"); node; node = node.next_sibling("char"))
		{
			int character = node.attribute("id").as_int();

			CharacterData data;
			data.x = node.attribute("x").as_int();
			data.y = node.attribute("y").as_int();
			data.width = node.attribute("width").as_int();
			data.height = node.attribute("height").as_int();
			data.offsetX = node.attribute("xoffset").as_int();
			data.offsetY = node.attribute("yoffset").as_int();
			data.advanceX = node.attribute("xadvance").as_int();

			font->characters[character] = data;
		}

		fonts[fileName] = font;
		return font;
	}

	DeminaAnimation* AssetManager::LoadDeminaAnimation(const std::string &fileName, Texture *texture, TextureDictionary *textureDictionary)
	{
		auto iterator = deminaAnimations.find(fileName);
		if (iterator != deminaAnimations.end())
		{
			return iterator->second;
		}
		else
		{
			int firstBoneCount = -1;

			pugi::xml_document document;
			pugi::xml_parse_result result = document.load_file((assetPath + fileName).c_str());
			
			if(result == true)
			{
				DeminaAnimation *deminaAnimation = new DeminaAnimation();
				
				pugi::xml_node animNode = document.child("Animation");
				if(!animNode)
					throw std::runtime_error(std::string("Failed to load animation: ") + fileName);
				
				deminaAnimation->frameRate = boost::lexical_cast<int>(animNode.child("FrameRate").child_value());
				float invFrameRate = 1.0f / deminaAnimation->frameRate;
				
				deminaAnimation->loopFrame = boost::lexical_cast<int>(animNode.child("LoopFrame").child_value());
				deminaAnimation->loopTime = invFrameRate * deminaAnimation->loopFrame;
				
				deminaAnimation->texture = texture;

				for(pugi::xml_node textureNode = animNode.child("Texture"); textureNode; textureNode = textureNode.next_sibling("Texture"))
				{
					std::string textureName = textureNode.child_value();
					deminaAnimation->textureRegions.push_back(textureDictionary->GetRegion(textureName));
				}

				for(pugi::xml_node keyNode = animNode.child("Keyframe"); keyNode; keyNode = keyNode.next_sibling("Keyframe"))
				{
					Keyframe keyframe;
					
					keyframe.VerticalFlip = keyNode.attribute("vflip").as_bool();
					keyframe.HorizontalFlip = keyNode.attribute("hflip").as_bool();
					keyframe.FrameNumber = keyNode.attribute("frame").as_int();
					keyframe.FrameTime = invFrameRate * keyframe.FrameNumber;
					keyframe.FrameTrigger = keyNode.attribute("trigger").value();
					
					for(pugi::xml_node boneNode = keyNode.child("Bone"); boneNode; boneNode = boneNode.next_sibling("Bone"))
					{
						Bone bone;
						
						bone.Name = boneNode.attribute("name").value();
						bone.Visible = std::string(boneNode.child("Hidden").child_value()) != std::string("False");
						bone.VerticalFlip = std::string(boneNode.child("TextureFlipVertical").child_value()) != std::string("False");
						bone.HorizontalFlip = std::string(boneNode.child("TextureFlipHorizontal").child_value()) != std::string("False");
						bone.TextureIndex = boost::lexical_cast<int>(boneNode.child("TextureIndex").child_value());
						bone.ParentIndex = boost::lexical_cast<int>(boneNode.child("ParentIndex").child_value());
						bone.Position.X() = boost::lexical_cast<float>(boneNode.child("Position").child("X").child_value());
						bone.Position.Y() = boost::lexical_cast<float>(boneNode.child("Position").child("Y").child_value());
						bone.Rotation = boost::lexical_cast<float>(boneNode.child("Rotation").child_value());
						
						keyframe.Bones.push_back(bone);
					}
					
					if(firstBoneCount != -1 && keyframe.Bones.size() != firstBoneCount)
						throw std::exception();
					else
						firstBoneCount = static_cast<int>(keyframe.Bones.size());
					
					deminaAnimation->keyframes.push_back(keyframe);
				}
				
				deminaAnimations[fileName] = deminaAnimation;
				return deminaAnimation;
			}
			else
			{
				throw std::runtime_error(std::string("Failed to load animation: ") + fileName);
			}
		}
	}

	SpriteAnimation* AssetManager::LoadSpriteAnimation(const std::string &fileName, Texture *texture, TextureDictionary *textureDictionary)
	{
		auto iterator = spriteAnimations.find(fileName);
		if (iterator != spriteAnimations.end())
		{
			return iterator->second;
		}
		else
		{
			pugi::xml_document document;
			pugi::xml_parse_result result = document.load_file((assetPath + fileName).c_str());
			
			if(result.status != pugi::status_ok)
				return nullptr;
			
			SpriteAnimation *spriteAnimation = new SpriteAnimation();
			
			pugi::xml_node animNode = document.child("SpriteAnimation");
			if(!animNode)
				throw std::runtime_error(std::string("Failed to load animation: ") + fileName);
			
			spriteAnimation->frameSeconds = boost::lexical_cast<float>(animNode.child("FrameSeconds").child_value());
			spriteAnimation->loop = std::string(animNode.child("Loop").child_value()) == std::string("true");
			
			spriteAnimation->texture = texture;
			
			for(pugi::xml_node frameNode = animNode.child("Frame"); frameNode; frameNode = frameNode.next_sibling("Frame"))
			{
				std::string textureName = frameNode.child_value();
				spriteAnimation->frames.push_back(textureDictionary->GetRegion(textureName));
			}
			
			spriteAnimations[fileName] = spriteAnimation;
			return spriteAnimation;
		}
	}

	GleedLevel* AssetManager::LoadGleedLevel(const std::string &fileName)
	{
		auto iterator = gleedLevels.find(fileName);
		if (iterator != gleedLevels.end())
		{
			return iterator->second;
		}
		else
		{
			pugi::xml_document document;
			pugi::xml_parse_result result = document.load_file((assetPath + fileName).c_str());

			if(!result)
				throw std::runtime_error(std::string("Failed to load Gleed level: ") + fileName);
			
			GleedLevel *gleedLevel = new GleedLevel();

			pugi::xml_node levelNode = document.child("Level");
			if(!levelNode)
				throw std::runtime_error(std::string("Failed to load Gleed level: ") + fileName);

			pugi::xml_node propertiesNode = levelNode.child("CustomProperties");
			if(propertiesNode)
			{
				for(pugi::xml_node propNode = propertiesNode.child("Property"); propNode; propNode = propNode.next_sibling("Property"))
				{
					if(std::string(propNode.attribute("Type").value()) == std::string("string"))
					{
						gleedLevel->customProperties[propNode.attribute("Name").value()] = propNode.child("string").child_value();
					}
				}
			}

			pugi::xml_node layersNode = levelNode.child("Layers");
			if(layersNode)
			{
				for(pugi::xml_node layerNode = layersNode.child("Layer"); layerNode; layerNode = layerNode.next_sibling("Layer"))
				{
					GleedLayer layer;

					pugi::xml_node propertiesNode = layerNode.child("CustomProperties");
					if(propertiesNode)
					{
						for(pugi::xml_node propNode = propertiesNode.child("Property"); propNode; propNode = propNode.next_sibling("Property"))
						{
							if(std::string(propNode.attribute("Type").value()) == std::string("string"))
							{
								layer.customProperties[propNode.attribute("Name").value()] = propNode.child("string").child_value();
							}
						}
					}

					layer.name = layerNode.attribute("Name").value();
					layer.visible = layerNode.attribute("Visible").as_bool();
					layer.scrollSpeed.X() = boost::lexical_cast<float>(layerNode.child("ScrollSpeed").child("X").child_value());
					layer.scrollSpeed.Y() = boost::lexical_cast<float>(layerNode.child("ScrollSpeed").child("Y").child_value());

					pugi::xml_node itemsNode = layerNode.child("Items");
					if(itemsNode)
					{
						for(pugi::xml_node itemNode = itemsNode.child("Item"); itemNode; itemNode = itemNode.next_sibling("Item"))
						{
							std::shared_ptr<GleedItem> item;

							std::string type = itemNode.attribute("xsi:type").value();
							if(type == "TextureItem")
							{
								item.reset(new GleedTextureItem());
								GleedTextureItem *ti = dynamic_cast<GleedTextureItem*>(item.get());

								ti->flipHorizontally = std::string(itemNode.child("FlipHorizontally").child_value()) == "true";
								ti->flipVertically = std::string(itemNode.child("FlipVertically").child_value()) == "true";
								ti->origin.X() = boost::lexical_cast<float>(itemNode.child("Origin").child("X").child_value());
								ti->origin.Y() = boost::lexical_cast<float>(itemNode.child("Origin").child("Y").child_value());
								ti->rotation = boost::lexical_cast<float>(itemNode.child("Rotation").child_value());
								ti->scale.X() = boost::lexical_cast<float>(itemNode.child("Scale").child("X").child_value());
								ti->scale.Y() = boost::lexical_cast<float>(itemNode.child("Scale").child("Y").child_value());
								ti->textureFileName = itemNode.child("texture_filename").child_value();
								replace(ti->textureFileName.begin(), ti->textureFileName.end(), '\\', '/');
								ti->tintColor.R() = boost::lexical_cast<float>(itemNode.child("TintColor").child("R").child_value()) / 255.0f;
								ti->tintColor.G() = boost::lexical_cast<float>(itemNode.child("TintColor").child("G").child_value()) / 255.0f;
								ti->tintColor.B() = boost::lexical_cast<float>(itemNode.child("TintColor").child("B").child_value()) / 255.0f;
								ti->tintColor.A() = boost::lexical_cast<float>(itemNode.child("TintColor").child("A").child_value()) / 255.0f;
							}
							else if(type == "PathItem")
							{
								item.reset(new GleedPathItem());
								GleedPathItem *pi = dynamic_cast<GleedPathItem*>(item.get());

								pi->isPolygon = std::string(itemNode.child("IsPolygon").child_value()) == "true";
								pi->lineColor.R() = boost::lexical_cast<float>(itemNode.child("LineColor").child("R").child_value()) / 255.0f;
								pi->lineColor.G() = boost::lexical_cast<float>(itemNode.child("LineColor").child("G").child_value()) / 255.0f;
								pi->lineColor.B() = boost::lexical_cast<float>(itemNode.child("LineColor").child("B").child_value()) / 255.0f;
								pi->lineColor.A() = boost::lexical_cast<float>(itemNode.child("LineColor").child("A").child_value()) / 255.0f;
								pi->lineWidth = boost::lexical_cast<float>(itemNode.child("LineWidth").child_value());

								pugi::xml_node localItemsNode = itemNode.child("LocalPoints");
								for(pugi::xml_node localNode = localItemsNode.child("Vector2"); localNode; localNode = localNode.next_sibling("Vector2"))
								{
									NinjaParty::Vector2 v;
									v.X() = boost::lexical_cast<float>(localNode.child("X").child_value());
									v.Y() = boost::lexical_cast<float>(localNode.child("Y").child_value());
									pi->localPoints.push_back(v);
								}

								pugi::xml_node worldItemsNode = itemNode.child("WorldPoints");
								for(pugi::xml_node localNode = worldItemsNode.child("Vector2"); localNode; localNode = localNode.next_sibling("Vector2"))
								{
									NinjaParty::Vector2 v;
									v.X() = boost::lexical_cast<float>(localNode.child("X").child_value());
									v.Y() = boost::lexical_cast<float>(localNode.child("Y").child_value());
									pi->worldPoints.push_back(v);
								}
							}
							else if(type == "RectangleItem")
							{
								item.reset(new GleedRectangleItem());
								GleedRectangleItem *ri = dynamic_cast<GleedRectangleItem*>(item.get());

								ri->fillColor.R() = boost::lexical_cast<float>(itemNode.child("FillColor").child("R").child_value()) / 255.0f;
								ri->fillColor.G() = boost::lexical_cast<float>(itemNode.child("FillColor").child("G").child_value()) / 255.0f;
								ri->fillColor.B() = boost::lexical_cast<float>(itemNode.child("FillColor").child("B").child_value()) / 255.0f;
								ri->fillColor.A() = boost::lexical_cast<float>(itemNode.child("FillColor").child("A").child_value()) / 255.0f;
								ri->width = boost::lexical_cast<float>(itemNode.child("Width").child_value());
								ri->height = boost::lexical_cast<float>(itemNode.child("Height").child_value());
							}
							else if(type == "CircleItem")
							{
								item.reset(new GleedCircleItem());
								GleedCircleItem *ci = dynamic_cast<GleedCircleItem*>(item.get());

								ci->fillColor.R() = boost::lexical_cast<float>(itemNode.child("FillColor").child("R").child_value()) / 255.0f;
								ci->fillColor.G() = boost::lexical_cast<float>(itemNode.child("FillColor").child("G").child_value()) / 255.0f;
								ci->fillColor.B() = boost::lexical_cast<float>(itemNode.child("FillColor").child("B").child_value()) / 255.0f;
								ci->fillColor.A() = boost::lexical_cast<float>(itemNode.child("FillColor").child("A").child_value()) / 255.0f;
								ci->radius = boost::lexical_cast<float>(itemNode.child("Radius").child_value());
							}
							else
							{
								throw std::runtime_error(std::string("Unexpected item type in GleedLevel ") + fileName);
							}

							item->name = itemNode.attribute("Name").value();
							item->visible = itemNode.attribute("Visible").as_bool();
							item->position.X() = boost::lexical_cast<float>(itemNode.child("Position").child("X").child_value());
							item->position.Y() = boost::lexical_cast<float>(itemNode.child("Position").child("Y").child_value());

							pugi::xml_node propertiesNode = itemNode.child("CustomProperties");
							if(propertiesNode)
							{
								for(pugi::xml_node propNode = propertiesNode.child("Property"); propNode; propNode = propNode.next_sibling("Property"))
								{
									if(std::string(propNode.attribute("Type").value()) == std::string("string"))
									{
										item->customProperties[propNode.attribute("Name").value()] = propNode.child("string").child_value();
									}
								}
							}

							layer.items.push_back(item);
						}
					}

					gleedLevel->layers.push_back(layer);
				}
			}

			gleedLevels[fileName] = gleedLevel;
			return gleedLevel;
		}
	}

	SpineSkeletonData* AssetManager::LoadSpineSkeletonData(const std::string &fileName, TextureDictionary *textureDictionary)
	{
		auto iterator = spineSkeletons.find(fileName);
		
		if(iterator != spineSkeletons.end())
			return iterator->second;
		
		SpineSkeletonData *spineSkeletonData = new SpineSkeletonData();
		spineSkeletonData->skeletonLoader = new SpineSkeletonLoader(textureDictionary);
        
        std::ifstream skeletonFile(assetPath + fileName);        
		spineSkeletonData->skeletonData = spineSkeletonData->skeletonLoader->readSkeletonData(skeletonFile);
		
		spineSkeletons[fileName] = spineSkeletonData;
		return spineSkeletonData;
	}
	
	SpineAnimation* AssetManager::LoadSpineAnimation(const std::string &fileName, SpineSkeletonData *skeletonData)
	{
		auto iterator = spineAnimations.find(fileName);
		
		if(iterator != spineAnimations.end())
			return iterator->second;
        
        std::ifstream animationFile(assetPath + fileName);
        SpineAnimation *animation = skeletonData->skeletonLoader->readAnimation(animationFile, skeletonData->skeletonData);
        
        spineAnimations[fileName] = animation;
        return animation;
	}
}