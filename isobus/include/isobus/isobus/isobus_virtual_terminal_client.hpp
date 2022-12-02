//================================================================================================
/// @file isobus_virtual_terminal_client.hpp
///
/// @brief A class to manage a client connection to a ISOBUS virtual terminal display
/// @author Adrian Del Grosso
///
/// @copyright 2022 Adrian Del Grosso
//================================================================================================
#ifndef ISOBUS_VIRTUAL_TERMINAL_CLIENT_HPP
#define ISOBUS_VIRTUAL_TERMINAL_CLIENT_HPP

#include "isobus/isobus/can_internal_control_function.hpp"
#include "isobus/isobus/can_partnered_control_function.hpp"
#include "isobus/utility/processing_flags.hpp"

#include <memory>
#include <string>
#include <thread>
#include <vector>

namespace isobus
{
	//================================================================================================
	/// @class VirtualTerminalClient
	///
	/// @brief An client interface for interacting with a virtual terminal (VT) server
	/// @details This class is the main interface for working with a VT. To use it,
	/// you must instantiate it with a source and partner control function, and set and one or more
	/// object pools to this class to be uploaded to the VT server. Once this setup is done, call
	/// the initialize function to start running the internal state machine. The stack will take care
	/// of uploading the object pool, and then you will be able to interact with the pool using the
	/// provided "send" functions from your application.
	//================================================================================================
	class VirtualTerminalClient
	{
	public:
		/// @brief Enumerates the states that can be sent with a hide/show object command
		enum class HideShowObjectCommand : std::uint8_t
		{
			HideObject = 0, ///< Hides the object
			ShowObject = 1 ///< Shows an object
		};

		/// @brief Enumerates the states that can be sent with an enable/disable object command
		enum class EnableDisableObjectCommand : std::uint8_t
		{
			DisableObject = 0, ///< Disables a compatible object
			EnableObject = 1 ///< Enables a compatible object
		};

		/// @brief Enumerates the states that can be sent with a select input object options command
		enum class SelectInputObjectOptions : std::uint8_t
		{
			ActivateObjectForDataInput = 0x00, ///< Activates an object for data input
			SetFocusToObject = 0xFF ///< Focuses the object (usually this draws a temporary box around it)
		};

		/// @brief The different VT versions that a client or server might support
		enum class VTVersion
		{
			Version2OrOlder, ///< Client or server supports VT version 2 or lower
			Version3, ///< Client or server supports all of VT version 3
			Version4, ///< Client or server supports all of VT version 4
			Version5, ///< Client or server supports all of VT version 5
			Version6, ///< Client or server supports all of VT version 6
			ReservedOrUnknown, ///< Reserved value, not to be used
		};

		/// @brief Enumerates the different line directions that can be used when changing an endpoint of an object
		enum class LineDirection : std::uint8_t
		{
			TopLeftToBottomRightOfEnclosingVirtualRectangle = 0, ///< Draws the line from top left to bottom right of the enclosing virtual rectangle
			BottomLeftToTopRightOfEnclosingVirtualRectangle = 1 ///< Draws the line from bottom left to top right of the enclosing virtual rectangle
		};

		/// @brief Enumerates the different font sizes
		enum class FontSize : std::uint8_t
		{
			Size6x8 = 0, ///< 6x8 Font size
			Size8x8 = 1, ///< 8x8 Font size
			Size8x12 = 2, ///< 8x12 Font size
			Size12x16 = 3, ///< 12x16 Font size
			Size16x16 = 4, ///< 16x16 Font size
			Size16x24 = 5, ///< 16x24 Font size
			Size24x32 = 6, ///< 24x32 Font size
			Size32x32 = 7, ///< 32x32 Font size
			Size32x48 = 8, ///< 32x48 Font size
			Size48x64 = 9, ///< 48x64 Font size
			Size64x64 = 10, ///< 64x64 Font size
			Size64x96 = 11, ///< 64x96 Font size
			Size96x128 = 12, ///< 96x128 Font size
			Size128x128 = 13, ///< 128x128 Font size
			Size128x192 = 14 ///< 128x192 Font size
		};

		/// @brief Enumerates the font style options that can be encoded in a font style bitfield
		enum class FontStyleBits : std::uint8_t
		{
			Bold = 0, ///< Bold font style
			CrossedOut = 1, ///< Crossed-out font style (strikethrough)
			Underlined = 2, ///< Underlined font style
			Italic = 3, ///< Italic font style
			Inverted = 4, ///< Inverted font style (upside down)
			Flashing = 5, ///< Flashing font style
			FlashingHidden = 6, ///< Flashing between hidden and shown font style
			ProportionalFontRendering = 7 ///< Enables proportional font rendering if supported by the server
		};

		/// @brief Enumerates the different font types
		enum class FontType : std::uint8_t
		{
			ISO8859_1 = 0, ///< ISO Latin 1
			ISO8859_15 = 1, ///< ISO Latin 9
			ISO8859_2 = 2, ///< ISO Latin 2
			Reserved_1 = 3, ///< Reserved
			ISO8859_4 = 4, ///< ISO Latin 4
			ISO8859_5 = 5, ///< Cyrillic
			Reserved_2 = 6, ///< Reserved
			ISO8859_7 = 7, ///< Greek
			ReservedEnd = 239, ///< Reserved from ISO8859_7 to this value
			ProprietaryBegin = 240, ///< The beginning of the proprietary range
			ProprietaryEnd = 255 ///< The end of the proprietary region
		};

		/// @brief Enumerates the different fill types for an object
		enum class FillType : std::uint8_t
		{
			NoFill = 0, ///< No fill will be applied
			FillWithLineColor = 1, ///< Fill with the color of the outline of the shape
			FillWithSpecifiedColorInFillColorAttribute = 2, ///< Fill with the color specified by a fill attribute
			FillWithPatternGivenByFillPatternAttribute = 3 ///< Fill with a patter provided by a fill pattern attribute
		};

		/// @brief The types of object pool masks
		enum class MaskType : std::uint8_t
		{
			DataMask = 1, ///< A data mask, used in normal circumstances
			AlarmMask = 2 ///< An alarm mask, which has different metadata related to popping up alarms, like priority
		};

		/// @brief The allowable priorities of an alarm mask
		enum class AlarmMaskPriority : std::uint8_t
		{
			High = 0, ///< Overrides lower priority alarm masks
			Medium = 1, ///< Overrides low priority alarm masks
			Low = 2 ///< Overrides data masks
		};

		/// @brief Denotes the lock/unlock state of a mask. Used to freeze/unfreeze rendering of a mask.
		enum class MaskLockState : std::uint8_t
		{
			UnlockMask = 0, ///< Renders the mask normally
			LockMask = 1 ///< Locks the mask so rendering of it is not updated until it is unlocked or a timeout occurs
		};

		/// @brief The different key activation codes that a button press can generate
		enum class KeyActivationCode : std::uint8_t
		{
			ButtonUnlatchedOrReleased = 0, ///< Button is released
			ButtonPressedOrLatched = 1, ///< Button is pressed
			ButtonStillHeld = 2, ///< Button is being held down (sent cyclically)
			ButtonPressAborted = 3 ///< Press was aborted (user navigated away from the button and did not release it)
		};

		/// @brief The internal state machine state of the VT client
		enum class StateMachineState : std::uint8_t
		{
			Disconnected, ///< VT is not connected, and is not trying to connect yet
			WaitForPartnerVTStatusMessage, ///< VT client is initialized, waiting for a VT server to come online
			SendWorkingSetMasterMessage, ///< Client is sending the working state master message
			ReadyForObjectPool, ///< Client needs an object pool before connection can continue
			SendGetMemory, ///< Client is sending the "get memory" message to see if VT has enough memory available
			WaitForGetMemoryResponse, ///< Client is waiting for a response to the "get memory" message
			SendGetNumberSoftkeys, ///< Client is sending the "get number of soft keys" message
			WaitForGetNumberSoftKeysResponse, ///< Client is waiting for a response to the "get number of soft keys" message
			SendGetTextFontData, ///< Client is sending the "get text font data" message
			WaitForGetTextFontDataResponse, ///< Client is waiting for a response to the "get text font data" message
			SendGetHardware, ///< Client is sending the "get hardware" message
			WaitForGetHardwareResponse, ///< Client is waiting for a response to the "get hardware" message
			UploadObjectPool, ///< Client is uploading the object pool
			SendEndOfObjectPool, ///< Client is sending the end of object pool message
			WaitForEndOfObjectPoolResponse, ///< Client is waiting for the end of object pool response message
			Connected, ///< Client is connected to the VT server and the application layer is in control
			Failed ///< Client could not connect to the VT due to an error
		};

		/// @brief Enumerates the different events that can be associated with a macro
		enum class MacroEventID : std::uint8_t
		{
			Reserved = 0, ///< Reserved
			OnActivate = 1, ///< Event on activation of an object (such as for data input)
			OnDeactivate = 2, ///< Event on deactivation of an object
			OnShow = 3, ///< Event on an object being shown
			OnHide = 4, ///< Event on an object being hidden
			OnEnable = 5, ///< Event on enable of an object
			OnDisable = 6, ///< Event on disabling an object
			OnChangeActiveMask = 7, ///< Event on changing the active mask
			OnChangeSoftKeyMask = 8, ///< Event on change of the soft key mask
			OnChangeAttribute = 9, ///< Event on change of an attribute value
			OnChangeBackgroundColor = 10, ///< Event on change of a background color
			OnChangeFontAttributes = 11, ///< Event on change of a font attribute
			OnChangeLineAttributes = 12, ///< Event on change of a line attribute
			OnChangeFillAttributes = 13, ///< Event on change of a fill attribute
			OnChangeChildLocation = 14, ///< Event on change of a child objects location
			OnChangeSize = 15, ///< Event on change of an object size
			OnChangeValue = 16, ///< Event on change of an object value (like via `change numeric value`)
			OnChangePriority = 17, ///< Event on change of a mask's priority
			OnChangeEndPoint = 18, ///< Event on change of an object endpoint
			OnInputFieldSelection = 19, ///< Event when an input field is selected
			OnInputFieldDeselection = 20, ///< Event on deselection of an input field
			OnESC = 21, ///< Event on ESC (escape)
			OnEntryOfValue = 22, ///< Event on entry of a value
			OnEntryOfNewValue = 23, ///< Event on entry of a *new* value
			OnKeyPress = 24, ///< Event on the press of a key
			OnKeyRelease = 25, ///< Event on the release of a key
			OnChangeChildPosition = 26, ///< Event on changing a child object's position
			OnPointingEventPress = 27, ///< Event on a pointing event press
			OnPointingEventRelease = 28, ///< Event on a pointing event release
			ReservedBegin = 29, ///< Beginning of the reserved range
			ReservedEnd = 254, ///< End of the reserved range
			UseExtendedMacroReference = 255 ///< Use extended macro reference
		};

		/// @brief Enumerates the various VT server graphics modes
		enum class GraphicMode : std::uint8_t
		{
			Monochrome = 0, ///< Monochromatic graphics mode (1 bit)
			SixteenColour = 1, ///< 16 Color mode (4 bit)
			TwoHundredFiftySixColor = 2 ///< 256 Color mode (8 bit)
		};

		static constexpr std::uint16_t NULL_OBJECT_ID = 0xFFFF; ///< The NULL Object ID, usually drawn as blank space

		/// @brief The constructor for a VirtualTerminalClient
		/// @param[in] partner The VT server control function
		/// @param[in] clientSource The internal control function to communicate from
		VirtualTerminalClient(std::shared_ptr<PartneredControlFunction> partner, std::shared_ptr<InternalControlFunction> clientSource);

		/// @brief The destructor for the VirtualTerminalClient
		~VirtualTerminalClient();

		// Setup Functions
		/// @brief This function starts the state machine. Call this once you have supplied 1 or more object pool and are ready to connect.
		/// @param[in] spawnThread The client will start a thread to manage itself if this parameter is true. Otherwise you must update it cyclically.
		void initialize(bool spawnThread);

		/// @brief Returns if the client has been initialized
		/// @returns true if the client has been initialized
		bool get_is_initialized();

		// Calling this will stop the worker thread if it exists
		/// @brief Terminates the client and joins the worker thread if applicable
		void terminate();

		// Basic Interaction
		/// @brief A typedef for a generic key event for convenience
		typedef void (*VTKeyEventCallback)(KeyActivationCode keyEvent, std::uint8_t keyNumber, std::uint16_t objectID, std::uint16_t parentObjectID, VirtualTerminalClient *parentPointer);
		/// @brief A typedef for a generic pointing event, for convenience
		typedef void (*VTPointingEventCallback)(KeyActivationCode keyEvent, std::uint16_t xPosition, std::uint16_t yPosition, VirtualTerminalClient *parentPointer);
		/// @brief A typedef for a generic VT input object selection callback for convenience
		typedef void (*VTSelectInputObjectCallback)(std::uint16_t objectID, bool objectSelected, bool objectOpenForInput, VirtualTerminalClient *parentPointer);

		// Callbacks for events that happen on the VT
		/// @brief Allows you to register for a callback when a softkey is pressed or released
		/// @param[in] value The callback to register
		void RegisterVTSoftKeyEventCallback(VTKeyEventCallback value);

		/// @brief Allows you to remove a callback for when a softkey is pressed or released
		/// @param[in] value The callback to remove
		void RemoveVTSoftKeyEventCallback(VTKeyEventCallback value);

		/// @brief Allows you to register for a callback when a button is pressed or released
		/// @param[in] value The callback to register
		void RegisterVTButtonEventCallback(VTKeyEventCallback value);

		/// @brief Allows you to remove a callback when a button is pressed or released
		/// @param[in] value The callback to remove
		void RemoveVTButtonEventCallback(VTKeyEventCallback value);

		/// @brief Allows you to register for a callback when a pointing event is "pressed or released"
		/// @param[in] value The callback to register
		void RegisterVTPointingEventCallback(VTPointingEventCallback value);

		/// @brief Allows you to remove a callback when a pointing event is "pressed or released"
		/// @param[in] value The callback to remove
		void RemoveVTPointingEventCallback(VTPointingEventCallback value);

		/// @brief Allows you to register for a callback when an input object event is triggered
		/// @param[in] value The callback to register
		void RegisterVTSelectInputObjectEventCallback(VTSelectInputObjectCallback value);

		/// @brief Allows you to remove an input object event callback
		/// @param[in] value The callback to remove
		void RemoveVTSelectInputObjectEventCallback(VTSelectInputObjectCallback value);

		// Command Messages
		/// @brief Sends a hide/show object command
		/// @details This command is used to hide or show a Container object.
		/// This pertains to the visibility of the object as well as its
		/// remembered state.If the object cannot be displayed due to references to missing
		/// objects, the VT generates an error in the response.
		/// @param[in] objectID The ID of the target object
		/// @param[in] command The target hide/show state of the object
		/// @returns true if the message was sent successfully
		bool send_hide_show_object(std::uint16_t objectID, HideShowObjectCommand command);

		/// @brief Sends an enable/disable object command
		/// @details This command is used to enable or disable an input field object
		/// or a Button object and pertains to the accessibility of an input field
		/// object or Button object.This command is also used to enable or disable an Animation object.
		/// It is allowed to enable already enabled objects and to disable already disabled objects.
		/// @param[in] objectID The ID of the target object
		/// @param[in] command The target enable/disable state of the object
		/// @returns true if the message was sent successfully
		bool send_enable_disable_object(std::uint16_t objectID, EnableDisableObjectCommand command);

		/// @brief Sends a select input object command
		/// @details This command is used to force the selection of an input field, Button, or Key object.
		/// @param[in] objectID The ID of the target object
		/// @param[in] option The method by which the object will be selected
		/// @returns true if the message was sent successfully
		bool send_select_input_object(std::uint16_t objectID, SelectInputObjectOptions option);

		/// @brief Sends the ESC message (Escape)
		/// @returns true if the message was sent successfully
		bool send_ESC();

		/// @brief Sends the control audio signal command
		/// @details This command may be used to control the audio on the VT.
		/// When received this message shall terminate any audio in process from
		/// the originating ECU and replace the previous command with the new command.
		/// @param[in] activations Number of times to activate the signal
		/// @param[in] frequency_hz The audio frequency to command in Hz
		/// @param[in] duration_ms Duration of the signal activation
		/// @param[in] offTimeDuration_ms The amount of silent time in the signal
		/// @returns true if the message was sent successfully
		bool send_control_audio_signal(std::uint8_t activations, std::uint16_t frequency_hz, std::uint16_t duration_ms, std::uint16_t offTimeDuration_ms);

		/// @brief Sends the set audio volume command
		/// @details This command applies to subsequent Control Audio Signal commands.
		/// VTs that are not able to modify the volume of the currently playing tone shall set
		/// the Audio device is busy bit in the response.This command should not affect in any way
		/// the volume settings of other Working Sets and shall not affect the volume of Alarm Masks.
		/// @param[in] volume_percent The volume percentage to set the VT server to
		/// @returns true if the message was sent successfully
		bool send_set_audio_volume(std::uint8_t volume_percent);

		/// @brief Sends the change child location command
		/// @details The Change Child Location command is used to change the position of an object. The new position is set
		/// relative to the object's current position. Since the object can be included in many
		/// parent objects, the parent Object ID is also included. If a parent object includes
		/// the child object multiple times, then each instance will be moved.
		/// The position attributes given in the message have an offset of -127, so
		/// a value of 255 results in a +128 px move.
		/// Positive values indicate a position change down or to the right. Negative values
		/// indicate a position change up or to the left.
		/// @param[in] objectID The ID of the target object
		/// @param[in] parentObjectID The ID of the object's parent object
		/// @param[in] relativeXPositionChange The amount to change the X position by (px)
		/// @param[in] relativeYPositionChange The amount to change the Y position by (px)
		/// @returns true if the message was sent successfully
		bool send_change_child_location(std::uint16_t objectID, std::uint16_t parentObjectID, std::uint8_t relativeXPositionChange, std::uint8_t relativeYPositionChange);

		/// @brief Sends the change child position command
		/// @details The new position is set relative to the parent object's position.
		/// Since the object can be included in many parent objects, the parent Object ID
		/// is also included.If a parent object includes the child object multiples times,
		/// then each instance will be moved to the same location(the designer may want to
		/// use Change Child Location command to move all instances in a relative motion).
		/// When the object is moved, the parent object shall be refreshed.
		/// The position attributes given in the message are signed integer.
		/// Positive values indicate a position below(Y) or to the right of(X) the top left
		/// corner of the parent object.Negative values indicate a position above(Y) or to the
		/// left of(X) the top left corner of the parent object.
		/// @param[in] objectID The ID of the target object
		/// @param[in] parentObjectID The ID of the object's parent object
		/// @param[in] xPosition The new X position of the object (px)
		/// @param[in] yPosition The new Y position of the object (px)
		/// @returns true if the message was sent successfully
		bool send_change_child_position(std::uint16_t objectID, std::uint16_t parentObjectID, std::uint16_t xPosition, std::uint16_t yPosition);

		/// @brief Sends the change size command
		/// @details A value of 0 for width or height or both
		/// means that the object size is 0 and the object is not drawn.
		/// @param[in] objectID The ID of the target object
		/// @param[in] newWidth The new width of the object
		/// @param[in] newHeight The new height of the object
		/// @returns true if the message was sent successfully
		bool send_change_size_command(std::uint16_t objectID, std::uint16_t newWidth, std::uint16_t newHeight);

		/// @brief Sends the change background color command
		/// @param[in] objectID The ID of the target object
		/// @param[in] color The new background color of the object
		/// @returns true if the message was sent successfully
		bool send_change_background_colour(std::uint16_t objectID, std::uint8_t color);

		/// @brief Sends the change numeric value command
		/// @details The size of the object shall not be changed by this command. Only the object indicated in the
		/// command is to be changed, variables referenced by the object are not changed.
		/// @param[in] objectID The ID of the target object
		/// @param[in] value The new numeric value of the object
		/// @returns true if the message was sent successfully
		bool send_change_numeric_value(std::uint16_t objectID, std::uint32_t value);

		/// @brief Sends the change string value command
		/// @details The size of the object shall not be changed by this command. Only the object indicated in the
		/// command is to be changed, variables referenced by the object are not changed.
		/// The transferred string is allowed to be smaller than the length of the value attribute of the target object and in
		/// this case the VT shall pad the value attribute with space characters.
		/// @param[in] objectID The ID of the target object
		/// @param[in] stringLength The length of the string to be sent
		/// @param[in] value The string to be sent
		/// @returns true if the message was sent successfully
		bool send_change_string_value(std::uint16_t objectID, uint16_t stringLength, const char *value);

		/// @brief Sends the change string value command (with a c++ string instead of buffer + length)
		/// @details The size of the object shall not be changed by this command. Only the object indicated in the
		/// command is to be changed, variables referenced by the object are not changed.
		/// The transferred string is allowed to be smaller than the length of the value attribute of the target object and in
		/// this case the VT shall pad the value attribute with space characters.
		/// @param[in] objectID The ID of the target object
		/// @param[in] value The string to be sent
		/// @returns true if the message was sent successfully
		bool send_change_string_value(std::uint16_t objectID, const std::string &value);

		/// @brief Sends the change endpoint command, which changes the end of an output line
		/// @param[in] objectID The ID of the target object
		/// @param[in] width_px The width to change the output line to
		/// @param[in] height_px The height to change the output line to
		/// @param[in] direction The line direction (refer to output line object attributes)
		/// @returns true if the message was sent successfully
		bool send_change_endpoint(std::uint16_t objectID, std::uint16_t width_px, std::uint16_t height_px, LineDirection direction);

		/// @brief Sends the change font attributes command
		/// @details This command is used to change the Font Attributes in a Font Attributes object.
		/// @param[in] objectID The ID of the target object
		/// @param[in] color See the standard VT colour palette for more details
		/// @param[in] size Font size
		/// @param[in] type Font Type
		/// @param[in] styleBitfield The font style encoded as a bitfield
		/// @returns true if the message was sent successfully
		bool send_change_font_attributes(std::uint16_t objectID, std::uint8_t color, FontSize size, std::uint8_t type, std::uint8_t styleBitfield);

		/// @brief Sends the change line attributes command
		/// @details This command is used to change the Line Attributes in a Line Attributes object.
		/// @param[in] objectID The ID of the target object
		/// @param[in] color See the standard VT colour palette for more details
		/// @param[in] width The line width
		/// @param[in] lineArtBitmask The line art, encoded as a bitfield (See ISO11783-6 for details)
		/// @returns true if the message was sent successfully
		bool send_change_line_attributes(std::uint16_t objectID, std::uint8_t color, std::uint8_t width, std::uint16_t lineArtBitmask);

		/// @brief Sends the change fill attributes command
		/// @details This command is used to change the Fill Attributes in a Fill Attributes object.
		/// @param[in] objectID The ID of the target object
		/// @param[in] fillType The fill type
		/// @param[in] color See the standard VT colour palette for more details
		/// @param[in] fillPatternObjectID Object ID to a fill pattern or NULL_OBJECT_ID
		/// @returns true if the message was sent successfully
		bool send_change_fill_attributes(std::uint16_t objectID, FillType fillType, std::uint8_t color, std::uint16_t fillPatternObjectID);

		/// @brief Sends the change active mask command
		/// @details This command is used to change the active mask of a Working Set
		/// to either a Data Mask object or an Alarm Mask object.
		/// @param[in] workingSetObjectID The ID of the working set
		/// @param[in] newActiveMaskObjectID The object ID of the new active mask
		/// @returns true if the message was sent successfully
		bool send_change_active_mask(std::uint16_t workingSetObjectID, std::uint16_t newActiveMaskObjectID);

		/// @brief Sends the change softkey mask command
		/// @details This command is used to change the Soft Key Mask associated with a
		/// Data Mask object or an Alarm Mask object.
		/// @param[in] type The mask type, alarm or data
		/// @param[in] dataOrAlarmMaskObjectID The object ID of the target mask
		/// @param[in] newSoftKeyMaskObjectID The object ID of the new softkey mask
		/// @returns true if the message was sent successfully
		bool send_change_softkey_mask(MaskType type, std::uint16_t dataOrAlarmMaskObjectID, std::uint16_t newSoftKeyMaskObjectID);

		/// @brief Sends the change attribute command
		/// @details This command is used to change any attribute with an assigned Attribute ID.
		/// This message cannot be used to change strings.
		/// @param[in] objectID The ID of the target object
		/// @param[in] attributeID The attribute ID of the attribute being changed
		/// @param[in] value The new attribute value
		/// @returns true if the message was sent successfully
		bool send_change_attribute(std::uint16_t objectID, std::uint8_t attributeID, std::uint32_t value);

		/// @brief Sends the change priority command
		/// @details This command is used to change the priority of an Alarm Mask.
		/// This command causes the VT to evaluate the priority of all active masks and
		/// may cause a change to a different mask if the Alarm Mask being changed
		/// should either become the active Working Set and mask,
		/// or should no longer be the active Working Set and mask.
		/// @param[in] alarmMaskObjectID The object ID of the target alarm mask
		/// @param[in] priority The new priority for the mask
		/// @returns true if the message was sent successfully
		bool send_change_priority(std::uint16_t alarmMaskObjectID, AlarmMaskPriority priority);

		/// @brief Sends the change list item command
		/// @details This command is used to change a list item in an Input List object,
		/// Output List object, animation object, or external object definition object.
		/// NULL_OBJECT_ID will result in the list item being removed, but will not change the index
		/// order of the other list items.
		/// @param[in] objectID The object ID of the list
		/// @param[in] listIndex The index in the list to edit
		/// @param[in] newObjectID The new object ID for the specified list index, or NULL_OBJECT_ID.
		/// @returns true if the message was sent successfully
		bool send_change_list_item(std::uint16_t objectID, std::uint8_t listIndex, std::uint16_t newObjectID);

		/// @brief Sends the lock unlock mask command
		/// @details This command is used by a Working Set to disallow or allow
		/// screen refreshes at the VT for the visible Data Mask or User Layout Data Mask
		/// owned by the requesting Working Set.
		/// This message would be used when a series of changes need to be synchronized or made visually atomic.
		/// The mask may be unlocked if a a timeout occurs based on the timeout attribute of this message, or by
		/// several other methods outlined in ISO11783-6, such as "proprietary reasons".
		/// @param[in] state The target lock/unlock state
		/// @param[in] objectID The object ID of the target mask
		/// @param[in] timeout_ms The max time to lock the mask, or 0 for no timeout. Does not apply to unlock commands.
		/// @returns true if the message was sent successfully
		bool send_lock_unlock_mask(MaskLockState state, std::uint16_t objectID, std::uint16_t timeout_ms);

		/// @brief Sends the execute macro command
		/// @details This command is used to execute a Macro.
		/// @param[in] objectID The ID of the target object
		/// @returns true if the message was sent successfully
		bool send_execute_macro(std::uint16_t objectID);

		/// @brief Sends the change object label command
		/// @details This command is used by an ECU to change a label of an object.
		/// @param[in] objectID The ID of the target object
		/// @param[in] labelStringObjectID The label's object ID
		/// @param[in] fontType The font type or NULL_OBJECT_ID
		/// @param[in] graphicalDesignatorObjectID Object ID of an object to be used as a graphic representation of the object label or NULL_OBJECT_ID
		/// @returns true if the message was sent successfully
		bool send_change_object_label(std::uint16_t objectID, std::uint16_t labelStringObjectID, std::uint8_t fontType, std::uint16_t graphicalDesignatorObjectID);

		/// @brief Sends change polygon point command
		/// @details This command is used by a Working Set to modify a point in an Output Polygon object.
		/// @param[in] objectID The ID of the target object
		/// @param[in] pointIndex The index of the point in the polygon to edit
		/// @param[in] newXValue The new X axis value (px)
		/// @param[in] newYValue The new Y axis value (px)
		/// @returns true if the message was sent successfully
		bool send_change_polygon_point(std::uint16_t objectID, std::uint8_t pointIndex, std::uint16_t newXValue, std::uint16_t newYValue);

		/// @brief Sends the change polygon scale command
		/// @details This command is used by a Working Set to change the scale of a complete Output Polygon object. This
		/// message causes the value of the polygon points to be changed.
		/// @param[in] objectID The ID of the target object
		/// @param[in] widthAttribute New width attribute
		/// @param[in] heightAttribute New height attribute
		/// @returns true if the message was sent successfully
		bool send_change_polygon_scale(std::uint16_t objectID, std::uint16_t widthAttribute, std::uint16_t heightAttribute);

		/// @brief Sends the select color map or palette command
		/// @param[in] objectID The object to select
		/// @returns true if the message was sent successfully
		bool send_select_color_map_or_palette(std::uint16_t objectID);

		/// @brief Sends the execute extended macro command
		/// @details Executes an extended macro
		/// @param[in] objectID The object ID of the extended macro to execute
		/// @returns true if the message was sent successfully
		bool send_execute_extended_macro(std::uint16_t objectID);

		/// @brief Sends the select active working set command
		/// @param[in] NAMEofWorkingSetMasterForDesiredWorkingSet The NAME of the target working set master
		/// @returns true if the message was sent successfully
		bool send_select_active_working_set(std::uint64_t NAMEofWorkingSetMasterForDesiredWorkingSet);

		// Graphics Context Commands:
		/// @brief Sends the set graphics cursor command
		/// @details This command sets the graphics cursor X/Y attributes of the object.
		/// @param[in] objectID The ID of the target object
		/// @param[in] xPosition The new X position (px)
		/// @param[in] yPosition The new Y position (px)
		/// @returns true if the message was sent successfully
		bool send_set_graphics_cursor(std::uint16_t objectID, std::int16_t xPosition, std::int16_t yPosition);

		/// @brief Sends the move graphics cursor command
		/// @details This command alters the graphics cursor x/y attributes of the object
		/// by moving it relative to its current position.
		/// @param[in] objectID The ID of the target object
		/// @param[in] xOffset The new relative X offset of the cursor
		/// @param[in] yOffset The new relative Y offset of the cursor
		/// @returns true if the message was sent successfully
		bool send_move_graphics_cursor(std::uint16_t objectID, std::int16_t xOffset, std::int16_t yOffset);

		/// @brief Sends the set foreground color command
		/// @details This command modifies the foreground colour
		/// attribute.The graphics cursor is not moved.
		/// @param[in] objectID The ID of the target object
		/// @param[in] color See standard color palette, 0-255
		/// @returns true if the message was sent successfully
		bool send_set_foreground_colour(std::uint16_t objectID, std::uint8_t color);

		/// @brief Sends the set background color command
		/// @details This command modifies the background colour
		/// attribute.The graphics cursor is not moved.
		/// @param[in] objectID The ID of the target object
		/// @param[in] color See standard color palette, 0-255
		/// @returns true if the message was sent successfully
		bool send_set_background_colour(std::uint16_t objectID, std::uint8_t color);

		/// @brief Sends the set line attributes object id
		/// @details This command modifies the Output Line object
		/// attribute. All drawing commands that follow use the new attribute value.
		/// For line suppression, set the Object ID to NULL.
		/// The graphics cursor is not moved.
		/// @param[in] objectID The ID of the target object
		/// @param[in] lineAttributeobjectID The object ID of the line attribute
		/// @returns true if the message was sent successfully
		bool send_set_line_attributes_object_id(std::uint16_t objectID, std::uint16_t lineAttributeobjectID);

		/// @brief Sends the fill attributes object id
		/// @details This command modifies the fill object attribute. All
		/// drawing commands that follow use the new attribute value.
		/// For no filling, set the Object ID to NULL. The
		/// graphics cursor is not moved.
		/// @param[in] objectID The ID of the target object
		/// @param[in] fillAttributeobjectID The object ID of the fill attribute
		/// @returns true if the message was sent successfully
		bool send_set_fill_attributes_object_id(std::uint16_t objectID, std::uint16_t fillAttributeobjectID);

		/// @brief Sends the set fill attributes object ID command
		/// @details This command modifies the font object attribute. All
		/// drawing commands that follow use the new attribute value.
		/// If text is not being used, the object can be set to NULL.
		/// The graphics cursor is not moved.
		/// @param[in] objectID The ID of the target object
		/// @param[in] fontAttributesObjectID The object ID of the font attribute
		/// @returns true if the message was sent successfully
		bool send_set_font_attributes_object_id(std::uint16_t objectID, std::uint16_t fontAttributesObjectID);

		/// @brief Sends the erase rectangle command
		/// @details Fills the rectangle at the graphics cursor using the
		/// current background colour.For this command, the Fill Attributes Object is
		/// not used regardless of the state of Options bit 1 The graphics cursor is
		/// moved to the bottom right pixel inside of the rectangle.
		/// @param[in] objectID The ID of the target object
		/// @param[in] width The width of the rectangle
		/// @param[in] height The height of the rectangle
		/// @returns true if the message was sent successfully
		bool send_erase_rectangle(std::uint16_t objectID, std::uint16_t width, std::uint16_t height);

		/// @brief Sends the draw point command
		/// @details Sets the pixel to the foreground colour. The graphics
		/// cursor is moved to the defined point.
		/// @param[in] objectID The ID of the target object
		/// @param[in] xOffset The pixel X offset relative to the cursor
		/// @param[in] yOffset The pixel Y offset relative to the cursor
		/// @returns true if the message was sent successfully
		bool send_draw_point(std::uint16_t objectID, std::int16_t xOffset, std::int16_t yOffset);

		/// @brief Sends the draw line command
		/// @details Draws a line from the graphics cursor to the specified
		/// end pixel using the foreground colour. The Output Line
		/// Object drawing rules apply with respect to the end
		/// pixel location and Line Attributes.The graphics cursor
		/// is moved to the specified end pixel.
		/// @param[in] objectID The ID of the target object
		/// @param[in] xOffset The pixel X offset relative to the cursor
		/// @param[in] yOffset The pixel Y offset relative to the cursor
		/// @returns true if the message was sent successfully
		bool send_draw_line(std::uint16_t objectID, std::int16_t xOffset, std::int16_t yOffset);

		/// @brief Sends the draw rectangle command
		/// @details Draws a rectangle at the graphics cursor. The
		/// Rectangle Object drawing rules apply.If a Line
		/// Attributes object is currently defined, the border is
		/// drawn. If a fill attribute object is currently defined,
		/// the rectangle is filled.The graphics cursor is moved to the
		/// bottom right pixel inside of the rectangle.
		/// @param[in] objectID The ID of the target object
		/// @param[in] width The width of the rectangle (px)
		/// @param[in] height The height of the rectangle (px)
		/// @returns true if the message was sent successfully
		bool send_draw_rectangle(std::uint16_t objectID, std::uint16_t width, std::uint16_t height);

		/// @brief Sends the draw closed ellipse message
		/// @details Draws a closed ellipse bounded by the rectangle
		/// defined by the current graphics cursor location and the
		/// width and height given.The Output Ellipse object
		/// drawing rules apply.If a Line Attributes object is currently defined,
		/// the border is drawn.If a fill attribute object is currently defined,
		/// the ellipse is filled. The graphics cursor is moved to the bottom right pixel
		/// inside of the bounding rectangle.
		/// @param[in] objectID The ID of the target object
		/// @param[in] width The width of the ellipse (px)
		/// @param[in] height The height of the ellipse (px)
		/// @returns true if the message was sent successfully
		bool send_draw_closed_ellipse(std::uint16_t objectID, std::uint16_t width, std::uint16_t height);

		/// @brief Sends the draw polygon command
		/// @details Draws a polygon from the graphics cursor to the first
		/// point, then to the second point and so on. The polygon
		/// is closed if the last point has the offset 0,0. This is
		/// because offset 0,0 gives the coordinates of the original
		/// graphics cursor which was used as the first point in the
		/// polygon. If the data does not close the polygon, no
		/// automatic closing is performed and filling is ignored.
		/// Foreground colour is used for the border colour. The
		/// Output Polygon object drawing rules apply. If a Line
		/// Attributes object is currently defined, the border is
		/// drawn. If a fill object is currently defined and the
		/// polygon is closed, the polygon is filled. The graphics
		/// cursor is moved to the last point in the list.
		/// @param[in] objectID The ID of the target object
		/// @param[in] numberOfPoints Number of points in the polygon
		/// @param[in] listOfXOffsetsRelativeToCursor A list of X offsets for the points, relative to the cursor
		/// @param[in] listOfYOffsetsRelativeToCursor A list of Y offsets for the points, relative to the cursor
		/// @returns true if the message was sent successfully
		bool send_draw_polygon(std::uint16_t objectID, std::uint8_t numberOfPoints, std::int16_t *listOfXOffsetsRelativeToCursor, std::int16_t *listOfYOffsetsRelativeToCursor);

		/// @brief Sends the draw text command
		/// @details Draws the given text using the Font Attributes object.
		/// Any flashing bits in the Font style of the Font Attributes
		/// object are ignored If opaque, the background colour
		/// attribute is used.The graphics cursor is moved to the
		/// bottom right corner of the extent of the text.
		/// @param[in] objectID The ID of the target object
		/// @param[in] transparent Denotes if the text background is transparent
		/// @param[in] textLength String length
		/// @param[in] value A buffer to the text to draw with length `textLength`
		/// @returns true if the message was sent successfully
		bool send_draw_text(std::uint16_t objectID, bool transparent, std::uint8_t textLength, const char *value);

		/// @brief Sends the pan viewport command
		/// @details This command modifies the viewport X and Y
		/// attributes and forces a redraw of the object.This
		/// allows �panning� of the underlying object contents.
		/// The graphics cursor is not moved.
		/// @param[in] objectID The ID of the target object
		/// @param[in] xAttribute The viewport X attribute
		/// @param[in] yAttribute The viewport Y attribute
		/// @returns true if the message was sent successfully
		bool send_pan_viewport(std::uint16_t objectID, std::int16_t xAttribute, std::int16_t yAttribute);

		/// @brief Sends the zoom viewport command
		/// @details This command allows magnification of the viewport
		/// contents. See section on zooming for meaning of the
		/// zoom value. The graphics cursor is not moved.
		/// @param[in] objectID The ID of the target object
		/// @param[in] zoom Zoom value, -32.0 to 32.0
		/// @returns true if the message was sent successfully
		bool send_zoom_viewport(std::uint16_t objectID, float zoom);

		/// @brief Sends the pan and zoom viewport command
		/// @details This command allows both panning and zooming at the same time.
		/// @param[in] objectID The ID of the target object
		/// @param[in] xAttribute The viewport X attribute
		/// @param[in] yAttribute The viewport Y attribute
		/// @param[in] zoom Zoom value, -32.0 to 32.0
		/// @returns true if the message was sent successfully
		bool send_pan_and_zoom_viewport(std::uint16_t objectID, std::int16_t xAttribute, std::int16_t yAttribute, float zoom);

		/// @brief Sends the change viewport size command
		/// @details This command changes the size of the viewport and
		/// can be compared to the normal Change Size
		/// command.The graphics cursor is not moved.
		/// @param[in] objectID The ID of the target object
		/// @param[in] width The width of the viewport
		/// @param[in] height The height of the viewport
		/// @returns true if the message was sent successfully
		bool send_change_viewport_size(std::uint16_t objectID, std::uint16_t width, std::uint16_t height);

		/// @brief Sends the draw VT object command
		/// @details his command draws the VT Object specified by the Object ID
		/// at the current graphics cursor location.
		/// @param[in] graphicsContextObjectID The ID of the target graphics context object
		/// @param[in] VTObjectID The object ID to draw
		/// @returns true if the message was sent successfully
		bool send_draw_vt_object(std::uint16_t graphicsContextObjectID, std::uint16_t VTObjectID);

		/// @brief Sends the copy canvas to picture graphic command
		/// @details This command copies the current canvas of the
		/// Graphics Context Object into the Picture Graphic object specified.
		/// @param[in] graphicsContextObjectID The ID of the target graphics context object
		/// @param[in] objectID The picture graphic's object ID to copy to
		/// @returns true if the message was sent successfully
		bool send_copy_canvas_to_picture_graphic(std::uint16_t graphicsContextObjectID, std::uint16_t objectID);

		/// @brief Sends the copy viewport to picture graphic command
		/// @details This command copies the current Viewport of the GCO into the
		/// specified picture graphic.
		/// @param[in] graphicsContextObjectID The ID of the target graphics context object
		/// @param[in] objectID The picture graphic's object ID to copy to
		/// @returns true if the message was sent successfully
		bool send_copy_viewport_to_picture_graphic(std::uint16_t graphicsContextObjectID, std::uint16_t objectID);

		// VT Querying
		/// @brief Sends the get attribute value message
		/// @param[in] objectID The object ID to query
		/// @param[in] attributeID The attribute object to query
		/// @returns true if the message was sent successfully
		bool send_get_attribute_value(std::uint16_t objectID, std::uint8_t attributeID);

		// Get Softkeys Response
		/// @brief Returns the number of X axis pixels in a softkey
		/// @returns The number of X axis pixels in a softkey
		std::uint8_t get_softkey_x_axis_pixels() const;

		/// @brief Returns the number of Y axis pixels in a softkey
		/// @returns The number of Y axis pixels in a softkey
		std::uint8_t get_softkey_y_axis_pixels() const;

		/// @brief Returns the number of virtual softkeys reported by the VT server
		/// @returns The number of virtual softkeys reported by the VT server
		std::uint8_t get_number_virtual_softkeys() const;

		/// @brief Returns the number of physical softkeys reported by the VT server
		/// @returns The number of physical softkeys reported by the VT server
		std::uint8_t get_number_physical_softkeys() const;

		// Get Text Font Data Response
		/// @brief Returns if the selected font is supported
		/// @param[in] value The font to check against
		/// @returns true if the font is supported by the VT server
		bool get_font_size_supported(FontSize value) const;

		/// @brief Returns if the selected font style is supported
		/// @param[in] value The font style to check against
		/// @returns true if the font style is supported by the VT server
		bool get_font_style_supported(FontStyleBits value) const;

		// Get Hardware Responses
		/// @brief Returns the graphics mode supported by the VT server
		/// @returns The graphics mode supported by the VT server
		GraphicMode get_graphic_mode() const;

		/// @brief Returns if the VT server supports a touchscreen with pointing message
		/// @returns true if the VT server supports a touchscreen with pointing message
		bool get_support_touchscreen_with_pointing_message() const;

		/// @brief Returns if the VT server supports a pointing device with pointing message
		/// @returns true if the VT server supports a pointing device with pointing message
		bool get_support_pointing_device_with_pointing_message() const;

		/// @brief Returns if the VT server supports multiple frequency audio output
		/// @returns true if the VT server supports multiple frequency audio output
		bool get_multiple_frequency_audio_output() const;

		/// @brief Returns if the VT server supports adjustable volume output
		/// @returns true if the VT server supports adjustable volume output
		bool get_has_adjustable_volume_output() const;

		/// @brief Returns if the VT server supports simultaneous activation of physical keys
		/// @returns true if the VT server supports simultaneous activation of physical keys
		bool get_support_simultaneous_activation_physical_keys() const;

		/// @brief Returns if the VT server supports simultaneous activation of buttons and softkeys
		/// @returns true if the VT server supports simultaneous activation of buttons and softkeys
		bool get_support_simultaneous_activation_buttons_and_softkeys() const;

		/// @brief Returns if the VT supports the drag operation
		/// @returns true if the VT supports the drag operation
		bool get_support_drag_operation() const;

		/// @brief Returns if the VT supports the intermediate coordinates during a drag operation
		/// @returns true if the VT supports the intermediate coordinates during a drag operation
		bool get_support_intermediate_coordinates_during_drag_operations() const;

		/// @brief Returns the number of x pixels in the data mask area
		/// @returns the number of x pixels in the data mask area
		std::uint16_t get_number_x_pixels() const;

		/// @brief Returns the number of y pixels in the data mask area
		/// @returns the number of y pixels in the data mask area
		std::uint16_t get_number_y_pixels() const;

		/// @brief Returns the VT version supported supported by the VT server
		/// @returns The VT version supported supported by the VT server
		VTVersion get_connected_vt_version() const;

		// ************************************************
		// Object Pool Interface
		// ************************************************
		// These are the functions for specifying your pool to upload.
		// You have a few options:
		// 1. Upload in one blob of contigious memory
		// This is good for small pools or pools where you have all the data in memory.
		// 2. Get a callback at some inteval to provide data in chunks
		// This is probably better for huge pools if you are RAM constrained, or if your
		// pool is stored on some external device that you need to get data from in pages.
		// This is also the best way to load from IOP files!
		// If using callbacks, The object pool and pointer MUST NOT be deleted or leave scope until upload is done.
		// Version must be the same for all pools uploaded to this VT server!!!

		/// @brief Assigns an object pool to the client using a buffer and size.
		/// @details This is good for small pools or pools where you have all the data in memory.
		/// @param[in] poolIndex The index of the pool you are assigning
		/// @param[in] poolSupportedVTVersion The VT version of the object pool
		/// @param[in] pool A pointer to the object pool. Must remain valid until client is connected!
		/// @param[in] size The object pool size
		void set_object_pool(std::uint8_t poolIndex, VTVersion poolSupportedVTVersion, const std::uint8_t *pool, std::uint32_t size);

		/// @brief Assigns an object pool to the client using a vector.
		/// @details This is good for small pools or pools where you have all the data in memory.
		/// @param[in] poolIndex The index of the pool you are assigning
		/// @param[in] poolSupportedVTVersion The VT version of the object pool
		/// @param[in] pool A pointer to the object pool. Must remain valid until client is connected!
		void set_object_pool(std::uint8_t poolIndex, VTVersion poolSupportedVTVersion, const std::vector<std::uint8_t> *pool);

		/// @brief Assigns an object pool to the client where the client will get data in chunks during upload.
		/// @details This is probably better for huge pools if you are RAM constrained, or if your
		/// pool is stored on some external device that you need to get data from in pages.
		/// This is also the best way to load from IOP files, as you can read the data in piece by piece.
		/// @param[in] poolIndex The index of the pool you are assigning
		/// @param[in] poolSupportedVTVersion The VT version of the object pool
		/// @param[in] poolTotalSize The object pool size
		/// @param[in] value The data callback that will be used to get object pool data to upload.
		void register_object_pool_data_chunk_callback(std::uint8_t poolIndex, VTVersion poolSupportedVTVersion, std::uint32_t poolTotalSize, DataChunkCallback value);

		/// @brief Periodic Update Function (worker thread may call this)
		/// @details This class can spawn a thread, or you can supply your own to run this function.
		/// To configure that behavior, see the initialize function.
		void update();

	private:
		/// @brief Enumerates the multiplexor byte values for VT commands
		enum class Function : std::uint8_t
		{
			SoftKeyActivationMessage = 0x00,
			ButtonActivationMessage = 0x01,
			PointingEventMessage = 0x02,
			VTSelectInputObjectMessage = 0x03,
			VTESCMessage = 0x04,
			VTChangeNumericValueMessage = 0x05,
			VTChangeActiveMaskMessage = 0x06,
			VTChangeSoftKeyMaskMessage = 0x07,
			VTChangeStringValueMessage = 0x08,
			VTOnUserLayoutHideShowMessage = 0x09,
			VTControlAudioSignalTerminationMessage = 0x0A,
			ObjectPoolTransferMessage = 0x11,
			EndOfObjectPoolMessage = 0x12,
			AuxiliaryAssignmentTypeOneCommand = 0x20,
			AuxiliaryInputTypeOneStatus = 0x21,
			PreferredAssignmentCommand = 0x22,
			AuxiliaryInputTypeTwoMaintenanceMessage = 0x23,
			AuxiliaryAssignmentTypeTwoCommand = 0x24,
			AuxiliaryInputStatusTypeTwoEnableCommand = 0x25,
			AuxiliaryInputTypeTwoStatusMessage = 0x26,
			AuxiliaryCapabilitiesRequest = 0x27,
			SelectActiveWorkingSet = 0x90,
			ESCCommand = 0x92,
			HideShowObjectCommand = 0xA0,
			EnableDisableObjectCommand = 0xA1,
			SelectInputObjectCommand = 0xA2,
			ControlAudioSignalCommand = 0xA3,
			SetAudioVolumeCommand = 0xA4,
			ChangeChildLocationCommand = 0xA5,
			ChangeSizeCommand = 0xA6,
			ChangeBackgroundColourCommand = 0xA7,
			ChangeNumericValueCommand = 0xA8,
			ChangeEndPointCommand = 0xA9,
			ChangeFontAttributesCommand = 0xAA,
			ChangeLineAttributesCommand = 0xAB,
			ChangeFillAttributesCommand = 0xAC,
			ChangeActiveMaskCommand = 0xAD,
			ChangeSoftKeyMaskCommand = 0xAE,
			ChangeAttributeCommand = 0xAF,
			ChangePriorityCommand = 0xB0,
			ChangeListItemCommand = 0xB1,
			DeleteObjectPoolCommand = 0xB2,
			ChangeStringValueCommand = 0xB3,
			ChangeChildPositionCommand = 0xB4,
			ChangeObjectLabelCommand = 0xB5,
			ChangePolygonPointCommand = 0xB6,
			ChangePolygonScaleCommand = 0xB7,
			GraphicsContextCommand = 0xB8,
			GetAttributeValueMessage = 0xB9,
			SelectColourMapCommand = 0xBA,
			IdentifyVTMessage = 0xBB,
			ExecuteExtendedMacroCommand = 0xBC,
			LockUnlockMaskCommand = 0xBD,
			ExecuteMacroCommand = 0xBE,
			GetMemoryMessage = 0xC0,
			GetSupportedWidecharsMessage = 0xC1,
			GetNumberOfSoftKeysMessage = 0xC2,
			GetTextFontDataMessage = 0xC3,
			GetWindowMaskDataMessage = 0xC4,
			GetSupportedObjectsMessage = 0xC5,
			GetHardwareMessage = 0xC7,
			StoreVersionCommand = 0xD0,
			LoadVersionCommand = 0xD1,
			DeleteVersionCommand = 0xD2,
			ExtendedGetVersionsMessage = 0xD3,
			ExtendedStoreVersionCommand = 0xD4,
			ExtendedLoadVersionCommand = 0xD5,
			ExtendedDeleteVersionCommand = 0xD6,
			GetVersionsMessage = 0xDF,
			GetVersionsResponse = 0xE0,
			UnsupportedVTFunctionMessage = 0xFD,
			VTStatusMessage = 0xFE,
			WorkingSetMaintenanceMessage = 0xFF
		};

		/// @brief Enumerates the command types for graphics context objects
		enum class GraphicsContextSubCommandID : std::uint8_t
		{
			SetGraphicsCursor = 0x00, ///< Sets the graphics cursor x/y attributes
			MoveGraphicsCursor = 0x01, ///< Moves the cursor relative to current location
			SetForegroundColor = 0x02, ///< Sets the foreground color
			SetBackgroundColor = 0x03, ///< Sets the background color
			SetLineAttributesObjectID = 0x04, ///< Sets the line attribute object ID
			SetFillAttributesObjectID = 0x05, ///< Sets the fill attribute object ID
			SetFontAttributesObjectID = 0x06, ///< Sets the font attribute object ID
			EraseRectangle = 0x07, ///< Erases a rectangle
			DrawPoint = 0x08, ///< Draws a point
			DrawLine = 0x09, ///< Draws a line
			DrawRectangle = 0x0A, ///< Draws a rectangle
			DrawClosedEllipse = 0x0B, ///< Draws a closed ellipse
			DrawPolygon = 0x0C, ///< Draws polygon
			DrawText = 0x0D, ///< Draws text
			PanViewport = 0x0E, ///< Pans viewport
			ZoomViewport = 0x0F, ///< Zooms the viewport
			PanAndZoomViewport = 0x10, ///< Pan and zooms the viewport
			ChangeViewportSize = 0x11, ///< Changes the viewport size
			DrawVTObject = 0x12, ///< Draws a VT object
			CopyCanvasToPictureGraphic = 0x13, ///< Copies the canvas to picture graphic object
			CopyViewportToPictureGraphic = 0x14 ///< Copies the viewport to picture graphic object
		};

		/// @brief Flags used as a retry mechanism for sending important messages
		enum class TransmitFlags : std::uint32_t
		{
			SendWorkingSetMaintenance = 0, ///< Flag to send the working set maintenenace message

			NumberFlags ///< The number of flags in this enum
		};

		/// @brief The different states of an object pool upload process
		enum class CurrentObjectPoolUploadState : std::uint8_t
		{
			Uninitialized, ///< The object pool upload has not been started
			InProgress, ///< The object pool upload is in progress
			Success, ///< The object pool was uploaded
			Failed ///< The pool upload has failed
		};

		/// @brief An object for storing information regarding an object pool upload
		struct ObjectPoolDataStruct
		{
			const std::uint8_t *objectPoolDataPointer; ///< A pointer to an object pool
			const std::vector<std::uint8_t> *objectPoolVectorPointer; ///< A pointer to an object pool (vector format)
			DataChunkCallback dataCallback; ///< A callback used to get data in chunks as an alternative to loading the whole pool at once
			std::uint32_t objectPoolSize; ///< The size of the object pool
			VTVersion version; ///< The version of the object pool. Must be the same for all pools!
			bool useDataCallback; ///< Determines if the client will use callbacks to get the data in chunks.
			bool uploaded; ///< The upload state of this pool
		};

		// Object Pool Managment
		/// @brief Sends the delete object pool message
		/// @returns true if the message was sent
		bool send_delete_object_pool();

		/// @brief Sends the working set maintenance message
		/// @param[in] initializing Used to set the initializing bit
		/// @param[in] workingSetVersion The version supported by the working set
		/// @returns true if the message was sent
		bool send_working_set_maintenance(bool initializing, VTVersion workingSetVersion);

		/// @brief Sends the get memory message
		/// @details This message checks to see if the VT has enough memory available to store your object pool(s)
		/// @param[in] requiredMemory Memory in bytes to check for on the VT server
		/// @returns true if the message was sent
		bool send_get_memory(std::uint32_t requiredMemory);

		/// @brief Sends the get number of softkeys message
		/// @returns true if the message was sent
		bool send_get_number_of_softkeys();

		/// @brief Sends the get text font data message
		/// @returns true if the message was sent
		bool send_get_text_font_data();

		/// @brief Sends the get hardware message
		/// @returns true if the message was sent
		bool send_get_hardware();

		/// @brief Sends the get supported widechars message
		/// @returns true if the message was sent
		bool send_get_supported_widechars();

		/// @brief Sends the get window mask data message
		/// @returns true if the message was sent
		bool send_get_window_mask_data();

		/// @brief Sends the get supported objects message
		/// @returns true if the message was sent
		bool send_get_supported_objects();

		/// @brief Sends the get versions message
		/// @returns true if the message was sent
		bool send_get_versions();

		/// @brief Sends the store version message
		/// @param[in] versionLabel The version label to store
		/// @returns true if the message was sent
		bool send_store_version(std::array<std::uint8_t, 7> versionLabel);

		/// @brief Sends the load version message
		/// @param[in] versionLabel The version label to load
		/// @returns true if the message was sent
		bool send_load_version(std::array<std::uint8_t, 7> versionLabel);

		/// @brief Sends the delete version message
		/// @param[in] versionLabel The version label to delete
		/// @returns true if the message was sent
		bool send_delete_version(std::array<std::uint8_t, 7> versionLabel);

		/// @brief Sends the get extended versions message
		/// @returns true if the message was sent
		bool send_extended_get_versions();

		/// @brief Sends the extended store version message
		/// @param[in] versionLabel The version label to store
		/// @returns true if the message was sent
		bool send_extended_store_version(std::array<std::uint8_t, 32> versionLabel);

		/// @brief Sends the extended load version message
		/// @param[in] versionLabel The version label to load
		/// @returns true if the message was sent
		bool send_extended_load_version(std::array<std::uint8_t, 32> versionLabel);

		/// @brief Sends the extended delete version message
		/// @param[in] versionLabel The version label to delete
		/// @returns true if the message was sent
		bool send_extended_delete_version(std::array<std::uint8_t, 32> versionLabel);

		/// @brief Sends the end of object pool message
		/// @returns true if the message was sent
		bool send_end_of_object_pool();

		/// @brief Sends the working set master message
		/// @returns true if the message was sent
		bool send_working_set_master();

		/// @brief Sets the state machine state and updates the associated timestamp
		/// @param[in] value The new state for the state machine
		void set_state(StateMachineState value);

		/// @brief Calls all registered callbacks for button events
		/// @param[in] keyEvent The button event
		/// @param[in] keyNumber They key number
		/// @param[in] objectID The object ID of the button
		/// @param[in] parentObjectID The object ID of the parent object
		/// @param[in] parentPointer A context variable that is passed back through the callback
		void process_button_event_callback(KeyActivationCode keyEvent, std::uint8_t keyNumber, std::uint16_t objectID, std::uint16_t parentObjectID, VirtualTerminalClient *parentPointer);

		/// @brief Calls all registered callbacks for softkey events
		/// @param[in] keyEvent The softkey event
		/// @param[in] keyNumber They key number
		/// @param[in] objectID The object ID of the softkey
		/// @param[in] parentObjectID The object ID of the parent object
		/// @param[in] parentPointer A context variable that is passed back through the callback
		void process_softkey_event_callback(KeyActivationCode keyEvent, std::uint8_t keyNumber, std::uint16_t objectID, std::uint16_t parentObjectID, VirtualTerminalClient *parentPointer);

		/// @brief Calls all registered callbacks for pointing events
		/// @param[in] signal The event's signal
		/// @param[in] xPosition The pointing event X position
		/// @param[in] yPosition The pointing event Y position
		/// @param[in] parentPointer A context variable that is passed back through the callback
		void process_pointing_event_callback(KeyActivationCode signal, std::uint16_t xPosition, std::uint16_t yPosition, VirtualTerminalClient *parentPointer);

		/// @brief Calls all registered callbacks for pointing events
		/// @param[in] objectID The object ID of the event's source object
		/// @param[in] objectSelected Denotes if the object is selected
		/// @param[in] objectOpenForInput Denotes if the input object is open for input
		/// @param[in] parentPointer A context variable that is passed back through the callback
		void process_select_input_object_callback(std::uint16_t objectID, bool objectSelected, bool objectOpenForInput, VirtualTerminalClient *parentPointer);

		/// @brief Processes the internal Tx flags
		/// @param[in] flag The flag to process
		/// @param[in] parent A context variable to find the relevant VT client class
		static void process_flags(std::uint32_t flag, void *parent);

		/// @brief Processes a CAN message destined for any VT client
		/// @param[in] message The CAN message being received
		/// @param[in] parentPointer A context variable to find the relevant VT client class
		static void process_rx_message(CANMessage *message, void *parentPointer);

		/// @brief The callback passed to the network manager's send function to know when a Tx is completed
		static void process_callback(std::uint32_t parameterGroupNumber,
		                             std::uint32_t dataLength,
		                             InternalControlFunction *sourceControlFunction,
		                             ControlFunction *destinationControlFunction,
		                             bool successful,
		                             void *parentPointer);

		/// @brief The data callback passed to the network manger's send function for the transport layer messages
		/// @details We upload the data with callbacks to avoid making a complete copy of the pool to
		/// accommodate the multiplexor that needs to get passed to the transport layer message's first byte.
		/// @param[in] callbackIndex The number of times the callback has been called
		/// @param[in] bytesOffset The byte offset at which to get pool data
		/// @param[in] numberOfBytesNeeded The number of bytes the protocol needs to send another frame (usually 7)
		/// @param[out] chunkBuffer A pointer through which the data should be returned to the protocol
		/// @param[in] parentPointer A context variable that is passed back through the callback
		/// @returns true if the data was successfully returned via the callback
		static bool process_internal_object_pool_upload_callback(std::uint32_t callbackIndex,
		                                                         std::uint32_t bytesOffset,
		                                                         std::uint32_t numberOfBytesNeeded,
		                                                         std::uint8_t *chunkBuffer,
		                                                         void *parentPointer);

		/// @brief The worker thread will execute this function when it runs, if applicable
		void worker_thread_function();

		static constexpr std::uint32_t VT_STATUS_TIMEOUT_MS = 3000; ///< The max allowable time between VT status messages before its considered offline
		static constexpr std::uint32_t WORKING_SET_MAINTENANCE_TIMEOUT_MS = 1000; ///< The frequency at which we send the working set maintenance message

		std::shared_ptr<PartneredControlFunction> partnerControlFunction; ///< The partner control function this client will send to
		std::shared_ptr<InternalControlFunction> myControlFunction; ///< The internal control function the client uses to send from

		ProcessingFlags txFlags; ///< A retry mechanism for internal Tx messages

		// Status message contents from the VT
		std::uint32_t lastVTStatusTimestamp_ms; ///< The timestamp of the last VT status message
		std::uint16_t activeWorkingSetDataMaskObjectID; ///< The active working set data mask object ID
		std::uint16_t activeWorkingSetSoftkeyMaskObjectID; ///< The active working set's softkey mask object ID
		std::uint8_t activeWorkingSetMasterAddress; ///< The active working set master address
		std::uint8_t busyCodesBitfield; ///< The VT server's busy codes
		std::uint8_t currentCommandFunctionCode; ///< The VT server's current command function code

		std::uint8_t connectedVTVersion; ///< The VT server's supported max version

		// Softkey capabilities
		std::uint8_t softKeyXAxisPixels; ///< The size of a soft key X dimension as reported by the VT server
		std::uint8_t softKeyYAxisPixels; ///< The size of a soft key Y dimension as reported by the VT server
		std::uint8_t numberVirtualSoftkeysPerSoftkeyMask; ///< The number of virtual softkeys per softkey mask as reported by the VT server
		std::uint8_t numberPhysicalSoftkeys; ///< The number of physical softkeys supported by the VT server

		// Text Font Capabilities
		std::uint8_t smallFontSizesBitfield; ///< The small font sizes supported by the VT server
		std::uint8_t largeFontSizesBitfield; ///< The large font sizes supported by the VT server
		std::uint8_t fontStylesBitfield; ///< The text font capabilities supported by the VT server

		// Hardware Capabilities, from the get hardware message
		GraphicMode supportedGraphicsMode; ///< The graphics mode reported by the VT server
		std::uint16_t xPixels; ///< The x pixel dimension as reported by the VT server
		std::uint16_t yPixels; ///< The y pixel dimension as reported by the VT server
		std::uint8_t hardwareFeaturesBitfield; ///< The reported hardware features from the VT server

		// Internal client state variables
		StateMachineState state; ///< The current client state machine state
		CurrentObjectPoolUploadState currentObjectPoolState; ///< The current upload state of the object pool being processed
		std::uint32_t stateMachineTimestamp_ms; ///< Timestamp from the last state machine update
		std::uint32_t lastWorkingSetMaintenanceTimestamp_ms; ///< The timestamp from the last time we sent the maintenance message
		std::vector<VTKeyEventCallback> buttonEventCallbacks; ///< A list of all button event callbacks
		std::vector<VTKeyEventCallback> softKeyEventCallbacks; ///< A list of all soft key event callbacks
		std::vector<VTPointingEventCallback> pointingEventCallbacks; ///< A list of all pointing event callbacks
		std::vector<VTSelectInputObjectCallback> selectInputObjectCallbacks; ///< A list of all select input object callbacks
		std::vector<ObjectPoolDataStruct> objectPools; ///< A container to hold all object pools that have been assigned to the interface
		std::thread *workerThread; ///< The worker thread that updates this interface
		bool initialized; ///< Stores the client initialization state
		bool sendWorkingSetMaintenenace; ///< Used internally to enable and disable cyclic sending of the maintenance message
		bool shouldTerminate; ///< Used to determine if the client should exit and join the worker thread

		// Object Pool info
		DataChunkCallback objectPoolDataCallback; ///< The callback to use to get pool data
		std::uint32_t objectPoolSize_bytes; ///< Total object pool size aggregate
		std::uint32_t lastObjectPoolIndex; ///< The last object pool index that was processed
	};

} // namespace isobus

#endif // ISOBUS_VIRTUAL_TERMINAL_CLIENT_HPP