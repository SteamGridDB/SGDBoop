#include "gui-helper.h"
#include <gtk/gtk.h>

int ShowMessageBox(const char* title, const char* message)
{
	gtk_init(NULL, NULL);

	GtkWidget* dialog = gtk_message_dialog_new(
		NULL,
		0,
		GTK_MESSAGE_ERROR,
		GTK_BUTTONS_OK,
		"%s",
		message
	);
	gtk_window_set_title(GTK_WINDOW(dialog), title);

	g_signal_connect_swapped (
		dialog,
		"response",
		G_CALLBACK (gtk_widget_destroy),
		dialog
	);

	return gtk_dialog_run(GTK_DIALOG(dialog));
}

enum {
	COLUMN_ITEM = 0,
	COLUMN_INDEX = 1,
};

static void button_callback(GtkWidget *widget, gpointer *data)
{
	GtkWidget* treeview = (GtkWidget*)data;

	GtkTreeSelection* selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview));

	GtkTreeModel *model;
	GtkTreeIter iter;
	if (gtk_tree_selection_get_selected(selection, &model, &iter))
	{
		int* selected_index = g_object_get_data(G_OBJECT(treeview), "selected_index");
		gtk_tree_model_get (model, &iter, COLUMN_INDEX, selected_index, -1);  
		gtk_main_quit();
	}
}

int SelectionDialog(const char* title, int count, const char** list, int selection)
{
	gtk_init(NULL, NULL);

	int selected_index = -1;

	// Setup a stylesheet to match the original UI
	const char* stylesheet = " \
		#box { \
			padding: 10px; \
		} \
	";

	GtkCssProvider* provider = gtk_css_provider_new();
	gtk_css_provider_load_from_data(provider, stylesheet, -1, NULL);

	GdkDisplay* display = gdk_display_get_default();
	GdkScreen* screen = gdk_display_get_default_screen(display);
	gtk_style_context_add_provider_for_screen(screen, GTK_STYLE_PROVIDER(provider),  GTK_STYLE_PROVIDER_PRIORITY_USER);

	// Put list into a GtkListStore
	GtkListStore *store = gtk_list_store_new (2, G_TYPE_STRING, G_TYPE_INT);

	GtkTreeIter iter;
	for (int i = 0; i < count; ++i) {
		gtk_list_store_append(store, &iter);
		gtk_list_store_set(store, &iter,
			COLUMN_ITEM, list[i],
			COLUMN_INDEX, i,
			-1);
	}

	// Dervice a TreeView from the ListStore
	GtkWidget *treeview = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
	// Attached the selected index to the treeview so we can easily reference it in the button callback
	g_object_set_data(G_OBJECT(treeview), "selected_index", &selected_index);
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(treeview), FALSE);

	// Change selection mode to single and set the current selected option if needed
	{    
		GtkTreeSelection* treeselection = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview));
		gtk_tree_selection_set_mode(treeselection, GTK_SELECTION_SINGLE);

		if (selection < count && selection >= 0) {
			GtkTreePath *path = gtk_tree_path_new_from_indices(selection, -1);
			gtk_tree_selection_select_path(treeselection, path);
			gtk_tree_path_free(path);
		}
	}

	// Define TreeView Cell Layout in the UI
	{    
		GtkCellRenderer* renderer = gtk_cell_renderer_text_new ();
		gtk_tree_view_insert_column_with_attributes (
			GTK_TREE_VIEW(treeview),
			-1,      
			"",  
			renderer,
			"text", COLUMN_ITEM,
			NULL
		);
	}

	// Put the TreeView into a ScrolledWindow
	GtkWidget* scrollable = gtk_scrolled_window_new(NULL, NULL);
	gtk_container_add(GTK_CONTAINER(scrollable), treeview);

	// Put a nice frame around the window to match the original 
	GtkWidget* frame = gtk_frame_new(NULL);
	gtk_container_add(GTK_CONTAINER(frame), scrollable);

	GtkWidget* window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(window), title);
	gtk_window_set_default_size(GTK_WINDOW(window), 454, 388);
	g_signal_connect(G_OBJECT(window), "destroy", gtk_main_quit, NULL);

	GtkWidget* box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
	gtk_widget_set_name(box, "box");
	gtk_container_add(GTK_CONTAINER(window), box);
	gtk_box_pack_start(GTK_BOX(box), frame, TRUE, TRUE, 0);

	GtkWidget* buttons = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
	gtk_box_pack_end(GTK_BOX(box), buttons, FALSE, FALSE, 0);
	gtk_widget_set_halign(buttons, GTK_ALIGN_END);
	gtk_widget_set_valign(buttons, GTK_ALIGN_START);

	GtkWidget* ok_button = gtk_button_new_with_label("OK");
	gtk_widget_set_size_request(ok_button, 75, 40);
	g_signal_connect(G_OBJECT(ok_button), "clicked", G_CALLBACK(button_callback), treeview);
	gtk_container_add(GTK_CONTAINER(buttons), ok_button);

	GtkWidget* cancel_button = gtk_button_new_with_label("Cancel");
	gtk_widget_set_size_request(cancel_button, 75, 40);
	g_signal_connect(G_OBJECT(cancel_button), "clicked", gtk_main_quit, NULL);
	gtk_container_add(GTK_CONTAINER(buttons), cancel_button);

	gtk_widget_show_all(window);
	gtk_main();

	return selected_index;
}